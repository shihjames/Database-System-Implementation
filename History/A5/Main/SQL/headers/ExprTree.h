
#ifndef SQL_EXPRESSIONS
#define SQL_EXPRESSIONS

#include "MyDB_AttType.h"
#include "MyDB_Catalog.h"
#include <string>
#include <vector>

// create a smart pointer for database tables
using namespace std;
class ExprTree;
typedef shared_ptr <ExprTree> ExprTreePtr;

// this class encapsules a parsed SQL expression (such as "this.that > 34.5 AND 4 = 5")

// class ExprTree is a pure virtual class... the various classes that implement it are below
class ExprTree {

public:
	virtual string toString () = 0;
	virtual ~ExprTree () {};
	virtual string getType() = 0;
	virtual bool check(MyDB_CatalogPtr myCatalog, vector<pair<string,string>> tables) = 0;
};

class BoolLiteral : public ExprTree {

private:
	bool myVal;
public:
	
	BoolLiteral (bool fromMe) {
		myVal = fromMe;
	}

	string toString () {
		if (myVal) {
			return "bool[true]";
		} else {
			return "bool[false]";
		}
	}	

	string getType(){
		return "BOOL";
	}

    bool check(MyDB_CatalogPtr myCatalog, vector<pair<string,string>> tablesIn) {
        return true;
    }
};

class DoubleLiteral : public ExprTree {

private:
	double myVal;
public:

	DoubleLiteral (double fromMe) {
		myVal = fromMe;
	}

	string toString () {
		return "double[" + to_string (myVal) + "]";
	}	

	~DoubleLiteral () {}

	string getType(){
		return "NUMBER";
	}

    bool check(MyDB_CatalogPtr myCatalog, vector<pair<string,string>> tablesIn) {
        return true;
    }
};

// this implement class ExprTree
class IntLiteral : public ExprTree {

private:
	int myVal;
public:

	IntLiteral (int fromMe) {
		myVal = fromMe;
	}

	string toString () {
		return "int[" + to_string (myVal) + "]";
	}

	~IntLiteral () {}

	string getType(){
		return "NUMBER";
	}

    bool check(MyDB_CatalogPtr myCatalog, vector<pair<string,string>> tablesIn) {
        return true;
    }
};

class StringLiteral : public ExprTree {

private:
	string myVal;
public:

	StringLiteral (char *fromMe) {
		fromMe[strlen (fromMe) - 1] = 0;
		myVal = string (fromMe + 1);
	}

	string toString () {
		return "string[" + myVal + "]";
	}

	~StringLiteral () {}

	string getType(){
		return "STRING";
	}

    bool check(MyDB_CatalogPtr myCatalog, vector<pair<string,string>> tablesIn) {
        return true;
    }
};

class Identifier : public ExprTree {

private:
	string type;
	string tableName;
	string attName;
public:

	Identifier (char *tableNameIn, char *attNameIn) {
		tableName = string (tableNameIn);
		attName = string (attNameIn);
		type = "IDENTIFIER";
	}

	string toString () {
		return "[" + tableName + "_" + attName + "]";
	}	

	~Identifier () {}

	string getType(){
		return type;
	}

    bool check(MyDB_CatalogPtr myCatalog, vector<pair<string,string>> tablesIn) {
		string tableCompare = "";
        for (auto t: tablesIn) {
            if (t.second == tableName) {
                tableCompare = t.first;
                break;
            }
        }
        if (tableCompare == "") {
			cout << "[ERROR] Table: " << tableName << " not exists." << endl;
            return false;
        }

        string attType;
        bool checkString = myCatalog->getString(tableCompare + "." + attName + ".type", attType);
		
        if(checkString == 0) {
			cout << "[ERROR] Attribute " << attName <<  "does not exist in Table " << tableCompare << endl;
            return false;
        }

        if(attType.compare("double") == 0 || attType.compare("int") == 0) {
            type = "NUMBER";
        }
        else if(attType.compare("string") == 0) {
            type = "STRING";
        }
        else {
            type = "BOOL";
        }

        return true;
    }
};

class MinusOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	MinusOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "- (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	~MinusOp () {}

	string getType(){
		return "NUMBER";
	}

    bool check(MyDB_CatalogPtr myCatalog, vector<pair<string,string>> tablesIn) {

		if(!rhs->check(myCatalog, tablesIn) || !lhs->check(myCatalog, tablesIn)){
            return false;
        }

        string lhsType = lhs->getType();
        string rhsType = rhs->getType();

		if(lhsType.compare("NUMBER") != 0){
			cout << "[ERROR] During Minus Operation, lhs: " << lhs->toString() << " is not numeric" << endl;
            return false;
        }

        if(rhsType.compare("NUMBER") != 0){
			cout << "[ERROR] During Minus Operation, rhs: " << rhs->toString() << " is not numeric" << endl;
            return false;
        }

        return true;
    }
};

class PlusOp : public ExprTree {

private:

  	string type;
	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	PlusOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		type = "NUMBER";
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "+ (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	string getType(){
		return type;
	}

    bool check(MyDB_CatalogPtr myCatalog, vector<pair<string,string>> tablesIn) {
        if(!rhs->check(myCatalog, tablesIn) || !lhs->check(myCatalog, tablesIn)){
            return false;
        }

        string lhsType = lhs->getType();
        string rhsType = rhs->getType();

        if(lhsType.compare(rhsType) != 0){
			cout << "[ERROR] During Plus Operation, the type of lhs and rhs are not the same" << endl;
            return false;
        }

        if(lhsType.compare("NUMBER") == 0) {
            return true;
        }
        else if(lhsType.compare("STRING") == 0) {
			type = "STRING";
            return true;
        }
        else {
			cout << "[ERROR] During Plus Operation, types of lhs and rhs should be either STRING or NUMBER!" << endl;
            return false;
        }

        return true;
    }

	~PlusOp () {}
};

class TimesOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	TimesOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "* (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	string getType(){
		return "NUMBER";
	}

	bool check(MyDB_CatalogPtr myCatalog, vector<pair<string,string>> tablesIn) {
        if(!rhs->check(myCatalog, tablesIn) || !lhs->check(myCatalog, tablesIn)){
            return false;
        }

        string lhsType = lhs->getType();
        string rhsType = rhs->getType();

        if(lhsType.compare("NUMBER") != 0){
			cout << "[ERROR] During Times Operation, lhs: " << lhs->toString() << " is not numeric" << endl;
            return false;
        }

        if(rhsType.compare("NUMBER") != 0){
            cout << "[ERROR] During Times Operation, rhs: " << rhs->toString() << " is not numeric" << endl;
            return false;
        }

        return true;
    }

	~TimesOp () {}
};

class DivideOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	DivideOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "/ (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	string getType(){
		return "NUMBER";
	}

	bool check(MyDB_CatalogPtr myCatalog, vector<pair<string,string>> tablesIn) {
        if (!rhs->check(myCatalog, tablesIn) || !lhs->check(myCatalog, tablesIn)){
            return false;
        }

		string lhsType = lhs->getType();
        string rhsType = rhs->getType();

        if (rhs->toString().compare("0") == 0) {
			cout << "[ERROR] During Divide Operation, rhs: " << rhs->toString() << " is zero!" << endl;
            return false;
        }

        if (lhsType.compare("NUMBER") != 0) {
			cout << "[ERROR] During Divide Operation, lhs: " << lhs->toString() << " is not numeric" << endl;
            return false;
        }

        if (rhsType.compare("NUMBER") != 0) {
			cout << "[ERROR] During Divide Operation, rhs: " << rhs->toString() << " is not numeric" << endl;
            return false;
        }

        return true;
    }

	~DivideOp () {}
};

class GtOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	GtOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "> (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	


	string getType(){
		return "BOOL";
	}

    bool check(MyDB_CatalogPtr myCatalog, vector<pair<string,string>> tablesIn) {
        if(!lhs->check(myCatalog, tablesIn) || !rhs->check(myCatalog, tablesIn)){
            return false;
        }

        string lhsType = lhs->getType();
        string rhsType = rhs->getType();

        if(lhsType.compare(rhsType) != 0){
			cout << "[ERROR] During Greater Compare Operation, the type of lhs and rhs are not the same" << endl;
            return false;
        }

        return true;
    }

	~GtOp () {}
};

class LtOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	LtOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "< (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	


	string getType(){
		return "BOOL";
	}

    bool check(MyDB_CatalogPtr myCatalog, vector<pair<string,string>> tablesIn) {
        if(!lhs->check(myCatalog, tablesIn) || !rhs->check(myCatalog, tablesIn)){
            return false;
        }

        string lhsType = lhs->getType();
        string rhsType = rhs->getType();

        if(lhsType.compare(rhsType) != 0){
        	cout << "[ERROR] During Less Compare Operation, the type of lhs and rhs are not the same" << endl;
            return false;
        }

        return true;
    }

	~LtOp () {}
};

class NeqOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	NeqOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "!= (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

		string getType(){
		return "BOOL";
	}

    bool check(MyDB_CatalogPtr myCatalog, vector<pair<string,string>> tablesIn) {
        if(!lhs->check(myCatalog, tablesIn) || !rhs->check(myCatalog, tablesIn)){
            return false;
        }

        string lhsType = lhs->getType();
        string rhsType = rhs->getType();

        if(lhsType.compare(rhsType) != 0){
            cout << "[ERROR] During Not Equal Compare Operation, the type of lhs and rhs are not the same" << endl;
            return false;
        }

        return true;
    }

	~NeqOp () {}
};

class OrOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	OrOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "|| (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	string getType(){
		return "BOOL";
	}

    bool check(MyDB_CatalogPtr myCatalog, vector<pair<string,string>> tablesIn) {
        if(!rhs->check(myCatalog, tablesIn) || !lhs->check(myCatalog, tablesIn)){
            return false;
        }

        string lhsType = lhs->getType();
        string rhsType = rhs->getType();

        if(lhsType.compare("BOOL") != 0){
			cout << "[ERROR] During OR Operation, the type of lhs " << lhs->toString() << " is not BOOL!" << endl;
            return false;
        }

        if(rhsType.compare("BOOL") != 0){
			cout << "[ERROR] During OR Operation, the type of rhs " << rhs->toString() << " is not BOOL!" << endl;
            return false;
        }

        return true;
    }

	~OrOp () {}
};

class EqOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	EqOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "== (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	string getType(){
		return "BOOL";
	}

    bool check(MyDB_CatalogPtr myCatalog, vector<pair<string,string>> tablesIn) {
        if(!lhs->check(myCatalog, tablesIn) || !rhs->check(myCatalog, tablesIn)){
            return false;
        }

        string lhsType = lhs->getType();
        string rhsType = rhs->getType();

        if(lhsType.compare(rhsType) != 0){
            cout << "[ERROR] During Equal Compare Operation, the type of lhs and rhs are not the same" << endl;
            return false;
        }

        return true;
    }

	~EqOp () {}
};

class NotOp : public ExprTree {

private:

	ExprTreePtr child;
	
public:

	NotOp (ExprTreePtr childIn) {
		child = childIn;
	}

	string toString () {
		return "!(" + child->toString () + ")";
	}	

	string getType(){
		return "BOOL";
	}

    bool check(MyDB_CatalogPtr myCatalog, vector<pair<string,string>> tablesIn) {
        if(!child->check(myCatalog, tablesIn)){
            return false;
        }

        string childType = child->getType();

        if(childType.compare("BOOL") != 0){
			cout << "[ERROR] During Not Operation, the type of child " << child->toString() << " is not BOOL!" << endl;
            return false;
        }

        return true;
    }

	~NotOp () {}
};

class SumOp : public ExprTree {

private:

	ExprTreePtr child;
	
public:

	SumOp (ExprTreePtr childIn) {
		child = childIn;
	}

	string toString () {
		return "sum(" + child->toString () + ")";
	}	

	string getType(){
		return "NUMBER";
	}

    bool check(MyDB_CatalogPtr myCatalog, vector<pair<string,string>> tablesIn) {
        if(!child->check(myCatalog, tablesIn)){
            return false;
        }

        string childType = child->getType();

        if(childType.compare("NUMBER") != 0){
			cout << "[ERROR] During SUM Operation, the type of child " << child->toString() << " is not numeric!" << endl;
            return false;
        }

        return true;
    }

	~SumOp () {}
};

class AvgOp : public ExprTree {

private:

	ExprTreePtr child;
	
public:

	AvgOp (ExprTreePtr childIn) {
		child = childIn;
	}

	string toString () {
		return "avg(" + child->toString () + ")";
	}	

	string getType(){
		return "NUMBER";
	}

    bool check(MyDB_CatalogPtr myCatalog, vector<pair<string,string>> tablesIn) {
        if(!child->check(myCatalog, tablesIn)){
            return false;
        }

        string childType = child->getType();

        if(childType.compare("NUMBER") != 0){
			cout << "[ERROR] During AVERAGE Operation, the type of child " << child->toString() << " is not numeric!" << endl;
            return false;
        }

        return true;
    }

	~AvgOp () {}
};

#endif
