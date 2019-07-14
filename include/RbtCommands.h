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

// Defines the interface for a token to be used by the parser

#ifndef _RBT_COMMANDS_H_
#define _RBT_COMMANDS_H_

class RbtCommands {
public:
  enum RbtCommName {
    ADD = 0,
    SUB = 1,
    MUL = 2,
    DIV = 3,
    IF = 4,
    LOG = 5,
    EXP = 6,
    RCTE = 7,
    AND = 8,
    NULLC = -1
  };

  ///////////////////
  // Constructors
  ///////////////////
  RbtCommands(const RbtCommands &c) : name(c.name), nargs(c.nargs) {}
  inline RbtCommands(int ncomm) {
    name = RbtCommName(ncomm);
    switch (name) {
    case ADD:
    case SUB:
    case MUL:
    case DIV:
    case AND:
      nargs = 2;
      break;
    case IF:
      nargs = 3;
      break;
    case LOG:
    case EXP:
      nargs = 1;
      break;
    case RCTE:
      nargs = 0;
      break;
    case NULLC:
      nargs = 0;
      break;
    }
  }

  inline RbtCommName GetName() const { return name; }

  inline int GetNArgs() const { return nargs; }

  inline bool IsAdd() const { return (name == ADD); }
  inline bool IsSub() const { return (name == SUB); }
  inline bool IsMul() const { return (name == MUL); }
  inline bool IsDiv() const { return (name == DIV); }
  inline bool IsIf() const { return (name == IF); }
  inline bool IsLog() const { return (name == LOG); }
  inline bool IsExp() const { return (name == EXP); }
  inline bool IsRCte() const { return (name == RCTE); }
  inline bool IsAnd() const { return (name == AND); }

  ///////////////////
  // Destructor
  //////////////////
  virtual ~RbtCommands() {}

  virtual void copy(const RbtCommands &c) {
    name = c.name;
    nargs = c.nargs;
  }

  ////////////////////
  // Private methods
  ////////////////////

private:
  RbtCommands(); // Default constructor disabled
  RbtCommName name;
  int nargs;
};

// Useful typedefs
// typedef SmartPtr<RbtCommands> RbtCommandsPtr;  //Smart pointer
// typedef std::vector<RbtCommandsPtr> RbtCommandsList;//Vector of smart
// pointers typedef RbtCommandsList::iterator RbtCommandsListIter; typedef
// RbtCommandsList::const_iterator RbtCommandsListConstIter;

// Compare class to compare different genomes inside a population
// This is used to sort the genomes in a population depending
// of the value of their scoring function.

#endif //_RbtCommands_H_
