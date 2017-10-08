/*
 * vcal.h
 *
 *  Created on: Jun 7, 2016
 *      Author: kkk
 */

#ifndef VCAL_H_
#define VCAL_H_
#include <iostream>
#include "definitions.h"
#include <sqlite3.h>
#include <fstream>
#include <stdint.h>
#include <stdlib.h>
#include <map>

namespace vcal {
using namespace std;

class CMB;
class Cframe;
class Cvideo;

struct SMV
{
    int x, y, scale, direction;
    //int MBw, MBh, MBx, MBy;
};

class CMV {
public:
	CMV();
	virtual ~CMV();
	void printInfo();
	SMV getMVvalues();
	void setStateReady(bool state);
	SMV MVs[2];
	CMB* parentMB;
	bool mReady;
	sqlite3 *db;
	sqlite3_stmt *stmt;
};

class CMB {
public:
	CMB();
	virtual ~CMB();
	CMB & operator=(const CMB &D );
	int32_t getMBType();
	string getMBTypeStr();
	int db_fillSubMBListHEVC(void);
	int fillMVs(void);
	void setStateReady(bool state);

	sqlite3 *db;
	sqlite3_stmt *stmt;
	CMV *mMV;
	CMB *mSubMBlist;
	int mNofSubMB;
	int mMBx;
	int mMBy;
	int mMBw;
	int mMBh;
	int mMBno;
	int mMBtype;
	int mPartMode;
	int mFrameNo;
	bool mReady;
	int mSpanRow;
	int mSpanColumn;
	Cframe* parentFrame;

};


class Cframe {
public:
	Cframe();
	virtual ~Cframe();
	int init(void);
	int allocateMBlist(void);
	int db_fetchAllFrameInfo(void);
	int fillMBlist(void);
	int db_fillSubMBList(void);
	int getNofSubMBsList(std::map<int, int> &NofSubMBsMap, sqlite3_stmt* myStmt);
	AVPictureType getPictureType();
	string getPictureTypeStr();
	CMB* MB(int,int);
	void setStateReady(bool state);
	int frameNo;
	AVPictureType pictureType;
	sqlite3 *db;
	sqlite3_stmt *stmt;
	bool mReady;
	Cvideo* parentVideo;
	int mMinBlockSize;
	int mMBlistSize;
	CMB *mMBlist;
	CMB *mCurrentMB;
	int mMinBlockWidth;
	int mMinBlockHeight;
	int width;
	int height;
	int codedPictureNumber;
};


class Cvideo {
public:
	Cvideo();
	Cvideo(string DBPath);
	virtual ~Cvideo();
	Cframe *frame(int);
	int db_fetchAllVideoInfo(void);
	void setStateReady(bool state); //Propagates the state
	//SQlite database variable
	sqlite3 *db;
	sqlite3_stmt *stmt;
	string pathToVideoInfoDB;
	bool mReady;
	AVCodecID mCodecID;
	char mCodecNameShort[256];
	char mCodecNameLong[1024];
	int mGOPSize;
	int mAspectRatioNum;
	int mAspectRatioDen;
private:
	bool is_file_exist(const char *fileName);
	int init(void);
	int openDB();
	int finilize_db(void);
	int getDBversion();
	Cframe *mFrame;

};

class vcal {
public:
	vcal();
	virtual ~vcal();
};

} /* namespace vcal */

#endif /* VCAL_H_ */
