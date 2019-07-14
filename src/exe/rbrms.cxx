/***********************************************************************
 * The rDock program was developed from 1998 - 2006 by the software team
 * at RiboTargets (subsequently Vernalis (R&D) Ltd).
 * In 2006, the software was licensed to the University of York for
 * maintenance and distribution.
 * In 2012, Vernalis and the University of York agreed to release the
 * program as Open Source software.
 * This version is licensed under GNU-LGPL version 3.0 with support from
 * the University of Barcelona.
 * http://rdock.sourceforge.net/
 ***********************************************************************/

// Calculates atomic RMSD of each SD record with record #1

#include <sstream>

#include "RbtMdlFileSink.h"
#include "RbtMdlFileSource.h"
#include "RbtModel.h"
#include "RbtModelError.h"

typedef std::vector<RbtCoordList> RbtCoordListList;
typedef RbtCoordListList::iterator RbtCoordListListIter;
typedef RbtCoordListList::const_iterator RbtCoordListListConstIter;

// Struct for holding symmetric bond params
class RbtSymBond {
public:
  RbtSymBond(RbtBondPtr bond, int n, bool swap)
      : m_bond(bond), m_n(n), m_swap(swap) {
    m_dih = (m_n > 0) ? 360.0 / m_n : 360.0;
  }
  RbtBondPtr m_bond; // The smart pointer to the bond itself
  int m_n;           // The symmetry operator (n-fold rotation)
  bool m_swap;       // false = spin atom 2 in bond; true = spin atom 1 in bond
  double m_dih;      // The dihedral step (360/n)
};

typedef SmartPtr<RbtSymBond> RbtSymBondPtr;
typedef std::vector<RbtSymBondPtr> RbtSymBondList;
typedef RbtSymBondList::iterator RbtSymBondListIter;
typedef RbtSymBondList::const_iterator RbtSymBondListConstIter;

// Class to enumerate all symmetry-related coordinate sets for a RbtModel
class EnumerateSymCoords {
public:
  EnumerateSymCoords(RbtModelPtr spModel);
  // Main method to get the sym coords
  void GetSymCoords(RbtCoordListList &cll);

private:
  void Setup();
  // Recursive operator to traverse the sym bond list
  void operator()(RbtSymBondListConstIter symIter, RbtCoordListList &cll);

  RbtModelPtr m_spModel;
  RbtCoordListList m_cll;
  RbtSymBondList m_symBondList;
  RbtAtomList m_heavyAtomList;
  RbtMolecularFileSinkPtr m_sink;
};

EnumerateSymCoords::EnumerateSymCoords(RbtModelPtr spModel)
    : m_spModel(spModel) {
  Setup();
  m_sink = new RbtMdlFileSink("rmsd_ref_sym.sd", m_spModel);
}

void EnumerateSymCoords::operator()(RbtSymBondListConstIter symIter,
                                    RbtCoordListList &cll) {
  // If we are not at the end of the sym bond list, then spin the current bond
  // At each dihedral step, recursively spin all remaining sym bonds
  if (symIter != m_symBondList.end()) {
    RbtSymBondPtr spSymBond = *symIter;
    for (int i = 0; i < spSymBond->m_n; i++) {
      m_spModel->RotateBond(spSymBond->m_bond, spSymBond->m_dih,
                            spSymBond->m_swap);
      (*this)(symIter + 1, cll); // Recursion
    }
  }
  // Once we get to the end of the sym bond list, append the conformation to the
  // ref coord list
  else {
    RbtCoordList coords;
    Rbt::GetCoordList(m_heavyAtomList, coords);
    cll.push_back(coords);
    m_sink->Render();
  }
  return;
}

// Main public method to enumerate the ref coords for the model
void EnumerateSymCoords::GetSymCoords(RbtCoordListList &cll) {
  cll.clear();
  (*this)(m_symBondList.begin(), cll);
}

void EnumerateSymCoords::Setup() {
  m_heavyAtomList = Rbt::GetAtomList(m_spModel->GetAtomList(),
                                     std::not1(Rbt::isAtomicNo_eq(1)));
  m_symBondList.clear();
  RbtBondList bondList = m_spModel->GetBondList();
  RbtStringList symBonds = m_spModel->GetDataValue("SYMMETRIC_BONDS");
  for (RbtStringListConstIter iter = symBonds.begin(); iter != symBonds.end();
       iter++) {
    int atomId1(1);
    int atomId2(1);
    int nSym(1);
    std::istringstream istr((*iter).c_str());
    istr >> atomId1 >> atomId2 >> nSym;
    bool bMatch = false;
    // Find the bond which matches these two atom IDs
    for (RbtBondListConstIter bIter = bondList.begin();
         bIter != bondList.end() && !bMatch; bIter++) {
      if (((*bIter)->GetAtom1Ptr()->GetAtomId() == atomId1) &&
          ((*bIter)->GetAtom2Ptr()->GetAtomId() == atomId2)) {
        RbtSymBondPtr spSymBond(new RbtSymBond(*bIter, nSym, false));
        m_symBondList.push_back(spSymBond);
        bMatch = true;
#ifdef _DEBUG
        std::cout << "Matched bond ID " << (*bIter)->GetBondId()
                  << " for atoms " << atomId1 << ", " << atomId2
                  << ", swap=false" << std::endl;
#endif //_DEBUG
      } else if (((*bIter)->GetAtom1Ptr()->GetAtomId() == atomId2) &&
                 ((*bIter)->GetAtom2Ptr()->GetAtomId() == atomId1)) {
        RbtSymBondPtr spSymBond(new RbtSymBond(*bIter, nSym, true));
        m_symBondList.push_back(spSymBond);
        bMatch = true;
#ifdef _DEBUG
        std::cout << "Matched bond ID " << (*bIter)->GetBondId()
                  << " for atoms " << atomId1 << ", " << atomId2
                  << ", swap=true" << std::endl;
#endif //_DEBUG
      }
    }
    if (bMatch == false) {
      std::cout << "Bond " << atomId1 << " - " << atomId2 << " not found"
                << std::endl;
    }
  }
}

// RMSD calculation between two coordinate lists
double rmsd(const RbtCoordList &rc, const RbtCoordList &c) {
  unsigned int nCoords = rc.size();
  if (c.size() != nCoords) {
    return 0.0;
  } else {
    double rms(0.0);
    for (unsigned int i = 0; i < nCoords; i++) {
      rms += Rbt::Length2(rc[i], c[i]);
    }
    rms = std::sqrt(rms / float(nCoords));
    return rms;
  }
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cout << "rbrms <ref sdfile> <input sdfile> [<output sdfile>] [<RMSD "
                 "threshold>]"
              << std::endl;
    std::cout
        << "RMSD is calculated for each record in <input sdfile> against <ref "
           "sdfile> (heavy atoms only)"
        << std::endl;
    std::cout
        << "If <output sdfile> is defined, records are written to output file "
           "with RMSD data field"
        << std::endl;
    std::cout
        << "If RMSD threshold is defined, records are removed which have an "
           "RMSD < threshold with any"
        << std::endl;
    std::cout << "previous record in <input sdfile>" << std::endl;
    return 1;
  }

  std::string strRefSDFile(argv[1]);
  std::string strInputSDFile(argv[2]);
  std::string strOutputSDFile;
  bool bOutput(false);
  if (argc > 3) {
    strOutputSDFile = argv[3];
    bOutput = true;
  }
  bool bRemoveDups(false);
  double threshold(1.0);
  if (argc > 4) {
    threshold = std::atof(argv[4]);
    bRemoveDups = true;
  }

  // std::ios_base::fmtflags oldflags = std::cout.flags();//save state
  std::ios_base::fmtflags oldflags = std::cout.flags(); // save state

  RbtDoubleList scoreVec;
  RbtDoubleList rmsVec;
  double minScore(9999.9);

  try {
    RbtMolecularFileSourcePtr spRefFileSource(new RbtMdlFileSource(
        Rbt::GetRbtFileName("data/ligands", strRefSDFile), false, false, true));
    // DM 16 June 2006 - remove any solvent fragments from reference
    // The largest fragment in each SD record always has segment name="H"
    // for reasons lost in the mists of rDock history
    spRefFileSource->SetSegmentFilterMap(Rbt::ConvertStringToSegmentMap("H"));
    // Get reference ligand (first record)
    RbtModelPtr spRefModel(new RbtModel(spRefFileSource));
    RbtCoordListList cll;
    EnumerateSymCoords symEnumerator(spRefModel);
    symEnumerator.GetSymCoords(cll);
    unsigned int nCoords = cll.front().size();

    std::cout << "molv_	rms rms	rmc rmc"
              << std::endl; // Dummy header line to be like do_anal

    std::cout.precision(3);
    std::cout.setf(std::ios_base::fixed, std::ios_base::floatfield);

    ///////////////////////////////////
    // MAIN LOOP OVER LIGAND RECORDS
    ///////////////////////////////////
    RbtMolecularFileSourcePtr spMdlFileSource(new RbtMdlFileSource(
        Rbt::GetRbtFileName("data/ligands", strInputSDFile), false, false,
        true));
    RbtMolecularFileSinkPtr spMdlFileSink;
    if (bOutput) {
      spMdlFileSink = new RbtMdlFileSink(strOutputSDFile, RbtModelPtr());
    }
    RbtModelList previousModels;
    for (int nRec = 1; spMdlFileSource->FileStatusOK();
         spMdlFileSource->NextRecord(), nRec++) {
      RbtError molStatus = spMdlFileSource->Status();
      if (!molStatus.isOK()) {
        std::cout << molStatus << std::endl;
        continue;
      }
      // DM 16 June 2006 - remove any solvent fragments from each record
      spMdlFileSource->SetSegmentFilterMap(Rbt::ConvertStringToSegmentMap("H"));
      RbtModelPtr spModel(new RbtModel(spMdlFileSource));
      RbtCoordList coords;
      Rbt::GetCoordList(Rbt::GetAtomList(spModel->GetAtomList(),
                                         std::not1(Rbt::isAtomicNo_eq(1))),
                        coords);

      if (coords.size() ==
          nCoords) { // Only calculate RMSD if atom count is same as reference
        double rms(9999.9);
        for (RbtCoordListListConstIter cIter = cll.begin(); cIter != cll.end();
             cIter++) {
          double rms1 = rmsd(*cIter, coords);
          // std::cout << "\tRMSD = " << rms1 << std::endl;
          rms = std::min(rms, rms1);
        }
        spModel->SetDataValue("RMSD", rms);
        double score = spModel->GetDataValue("SCORE");
        double scoreInter = spModel->GetDataValue("SCORE.INTER");
        double scoreIntra = spModel->GetDataValue("SCORE.INTRA");

        scoreVec.push_back(score);
        rmsVec.push_back(rms);
        minScore = std::min(minScore, score);

        bool bIsUnique(true);
        // Duplicate check
        if (bRemoveDups) {
          for (unsigned int i = 0; i < previousModels.size() && bIsUnique;
               i++) {
            RbtCoordList prevCoords;
            Rbt::GetCoordList(
                Rbt::GetAtomList(previousModels[i]->GetAtomList(),
                                 std::not1(Rbt::isAtomicNo_eq(1))),
                prevCoords);
            double rms0 = rmsd(prevCoords, coords);
            bIsUnique = (rms0 > threshold);
          }
        }
        // If we are not in 'remove duplicate' mode then bIsUnique is always
        // true
        if (bIsUnique) {
          std::cout << nRec << "\t" << score << "\t" << scoreInter << "\t"
                    << scoreIntra << "\t" << rms << "\t" << 0.0 << std::endl;
          if (bRemoveDups) {
            previousModels.push_back(spModel);
          }
          if (bOutput) {
            spMdlFileSink->SetModel(spModel);
            spMdlFileSink->Render();
          }
        }
      }
    }
    // END OF MAIN LOOP OVER LIGAND RECORDS
    ////////////////////////////////////////////////////
    RbtDoubleListConstIter sIter = scoreVec.begin();
    RbtDoubleListConstIter rIter = rmsVec.begin();
    double zTot(0.0);
    double zMean(0.0);
    double zMean2(0.0);
    // std::cout << std::endl << "Bolztmann-weighted RMSD calculation" <<
    // std::endl;
    for (; (sIter != scoreVec.end()) && (rIter != rmsVec.end());
         sIter++, rIter++) {
      double de = (*sIter) - minScore;
      double z = std::exp(-de / (8.314e-3 * 298.0));
      zTot += z;
      zMean += (*rIter) * z;
      zMean2 += (*rIter) * (*rIter) * z;
      // std::cout << *sIter << "\t" << de << "\t" << z << "\t" << zTot << "\t"
      // <<
      // (*rIter) << std::endl;
    }
    zMean /= zTot;
    // double zVar = zMean2 / zTot - (zMean * zMean);
    // std::cout << "zRMSD," << zTot << "," << zMean << "," << std::sqrt(zVar)
    // << std::endl;
  } catch (RbtError &e) {
    std::cout << e << std::endl;
  } catch (...) {
    std::cout << "Unknown exception" << std::endl;
  }

  std::cout.flags(oldflags); // Restore state

  _RBTOBJECTCOUNTER_DUMP_(std::cout)

  return 0;
}
