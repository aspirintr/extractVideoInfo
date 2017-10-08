//============================================================================
// Name        : vcal.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World
//============================================================================
#include "vcal.h"


namespace vcal {

vcal::vcal() {
	// TODO Auto-generated constructor stub
	std::cout<<"Hello vcal!\n";
}

vcal::~vcal() {
	// TODO Auto-generated destructor stub
}
////////Video
Cvideo::Cvideo() {
	pathToVideoInfoDB = "";
	init();
}

Cvideo::Cvideo(string DBPath) {
	pathToVideoInfoDB = DBPath;
	init();
}

Cvideo::~Cvideo() {
	finilize_db();
	delete mFrame;
	mFrame = NULL;
}
int Cvideo::getDBversion() {
    char sql[2048];
    int stepRes;
    int DBversion = -1;
	sprintf(sql,"SELECT version FROM DBinfo;");
	if(sqlite3_prepare_v2(db,sql,-1,&stmt,0)!=SQLITE_OK){
		cerr<<"Prepare statement returned error.\n";
		sqlite3_finalize(stmt);
		return -1;
	}
	stepRes = sqlite3_step(stmt);
	if ( !(stepRes==SQLITE_ROW || stepRes==SQLITE_DONE) ){
		cerr<<"Step statement returned error.\n";
		sqlite3_finalize(stmt);
		return -1;
	}
	if (stepRes==SQLITE_DONE){
		cerr<<"No such record.\n";
		sqlite3_finalize(stmt);
		return -1;
	}

	DBversion = sqlite3_column_int(stmt, 0);
	sqlite3_finalize(stmt);
	return DBversion;
}
int Cvideo::init() {
	mFrame = new Cframe;
	mFrame->db = db;
	//Connect to database
	if (!is_file_exist(pathToVideoInfoDB.c_str())){
		setStateReady(false);
		return VCAL_BAD_FILE;
	}
	if(openDB()!=VCAL_SUCCESSFUL)
		setStateReady(false);
	else{
		int DBversion = getDBversion();
		if (DBversion != SUPPORTED_VCAL_DATABASE_VERSION)
		{
			cerr<<"DB version ("<<SUPPORTED_VCAL_DATABASE_VERSION << ") is different than supported DB version ("<<DBversion << ").\n";
			setStateReady(false);
			return VCAL_ERR_DB_VERSION_IS_DIFFERENT;

		}
		db_fetchAllVideoInfo();
		setStateReady(true);
	}
	mFrame->db = db;
	return VCAL_SUCCESSFUL;
}
int Cvideo::db_fetchAllVideoInfo(void){
	char sql[2048];
	int stepRes;
	char* str;
	sprintf(sql,"SELECT  CodecID, CodecNameShort, CodecNameLong, GOPSize, AspectRatioNum, AspectRatioDen FROM VIDEO;");
	if(sqlite3_prepare_v2(db,sql,-1,&stmt,0)!=SQLITE_OK){
		cerr<<"Prepare statement returned error.\n";
		setStateReady(false);
		sqlite3_finalize(stmt);
		return -1;
	}
	stepRes = sqlite3_step(stmt);
	if ( !(stepRes==SQLITE_ROW || stepRes==SQLITE_DONE) ){
		cerr<<"Step statement returned error.\n";
		setStateReady(false);
		sqlite3_finalize(stmt);
		return -1;
	}
	if (stepRes==SQLITE_DONE){
		cerr<<"No such record.\n";
		sqlite3_finalize(stmt);
		setStateReady(false);
		return -1;
	}
	str = (char *)sqlite3_column_text(stmt, 1);
	sprintf(mCodecNameShort, "%s", str);
	str = (char *)sqlite3_column_text(stmt, 2);
	sprintf(mCodecNameLong, "%s", str);
	mCodecID =  (AVCodecID) sqlite3_column_int(stmt, 0);
	mGOPSize =  (AVPictureType) sqlite3_column_int(stmt, 3);
	mAspectRatioNum =  sqlite3_column_int(stmt, 4);
	mAspectRatioDen =  sqlite3_column_int(stmt, 5);

	setStateReady(true);
	sqlite3_finalize(stmt);

	return 0;
}
int Cvideo::openDB()
{
	int rc;
    rc = sqlite3_open(pathToVideoInfoDB.c_str(), &db);
    if( rc ){
       fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
       sqlite3_close(db);
       return VCAL_DATABASE_ERROR;
    }else{
       printf("Opened database successfully\n");
    }
	return VCAL_SUCCESSFUL;
}

int Cvideo::finilize_db(void){
	if (mReady){
		int ret=0;
		ret = sqlite3_close(db);
		if (ret){
			cerr<<"database could not be closed properly: "<<ret << " " << sqlite3_errmsg(db)<< endl;
		}
	}
	setStateReady(false);
	return 0;
}

void Cvideo::setStateReady(bool state){
	mReady = state;
	if (mFrame)
		mFrame->setStateReady(state);
}

Cframe* Cvideo::frame(int frameNo){
	if(!mReady){
		setStateReady(false);
		return  mFrame;
	}
	if (mFrame->frameNo != frameNo){
		mFrame->frameNo = frameNo;
		mFrame->db = db;
		mFrame->parentVideo = this;
		if (mFrame->init()){
			setStateReady(false);
			return mFrame;
		}
		setStateReady(true);//propagate state
	}
	return mFrame;
}

bool Cvideo::is_file_exist(const char *fileName)
{
    std::ifstream infile(fileName, ifstream::in);
    return infile.good();
}

//////////Frame
Cframe::Cframe(){
	frameNo=-1;
	mReady = false;
	pictureType = AV_PICTURE_TYPE_NONE;
	mCurrentMB = new CMB();
	mMBlist = NULL;
	mMBlistSize = 0;
	width = 0;
	height=0;
	mMinBlockHeight=0;
	mMinBlockWidth=0;
	mMinBlockSize=0;
	codedPictureNumber = 0;
	stmt = NULL;
	db = NULL;
	parentVideo = NULL;
}
Cframe::~Cframe(){
	delete mCurrentMB;
	if (mMBlist){
		delete[] mMBlist;
		mMBlist = NULL;
	}
}
int Cframe::init(void){
	if (db_fetchAllFrameInfo()){
		cerr<<"could not fetch all frame info.\n";
		setStateReady(false);
		return -1;
	}
	//TODO: if mMBlist, then free it.
	if (mMBlist){
		delete[] mMBlist;
		mMBlist = NULL;
	}
	if (fillMBlist()){
		setStateReady(false);
		return -1;
	}
	return 0;
}
int Cframe::allocateMBlist(void){
	//mMBlist = new CMB[mMinBlockWidth * mMinBlockHeight];
	return 0;
}
int Cframe::db_fetchAllFrameInfo(void){
	char sql[2048];
	int stepRes;
	if (mReady && frameNo >= 0){
		sprintf(sql,"SELECT  MinBlockWidth, MinBlockHeight, MinBlockSize, PictureType, Width, Height, CodedPictureNumber FROM FRAMEs WHERE FrameNumber=%d;",frameNo);
		if(sqlite3_prepare_v2(db,sql,-1,&stmt,0)!=SQLITE_OK){
			cerr<<"Prepare statement returned error.\n";
			setStateReady(false);
			sqlite3_finalize(stmt);
			return -1;
		}
		stepRes = sqlite3_step(stmt);
		if ( !(stepRes==SQLITE_ROW || stepRes==SQLITE_DONE) ){
			cerr<<"Step statement returned error.\n";
			setStateReady(false);
			sqlite3_finalize(stmt);
			return -1;
		}
		if (stepRes==SQLITE_DONE){
			cerr<<"No such record.\n";
			sqlite3_finalize(stmt);
			setStateReady(false);
			return -1;
		}
		mMinBlockWidth =  sqlite3_column_int(stmt, 0);
		mMinBlockHeight =  sqlite3_column_int(stmt, 1);
		mMinBlockSize =  sqlite3_column_int(stmt, 2);
		pictureType =  (AVPictureType)sqlite3_column_int(stmt, 3);
		width =  sqlite3_column_int(stmt, 4);
		height =  sqlite3_column_int(stmt, 5);
		codedPictureNumber =  sqlite3_column_int(stmt, 6);
		setStateReady(true);
		sqlite3_finalize(stmt);
	}

	return 0;
}

int Cframe::fillMBlist(void){
    if(!mCurrentMB){
    	mCurrentMB = new CMB;
    }
    int sqlite_ret=0;
	char sql[2048];
	int stepRes;
	if (mReady && frameNo >= 0){
		//First, get number of rows in the query result
		sprintf(sql,"SELECT COUNT(*) FROM (SELECT  MBno, MBx, MBy, MBw, MBh, MBtype, partMode FROM MBs WHERE FrameID=%d);",frameNo);
		if((sqlite_ret = sqlite3_prepare_v2(db,sql,-1,&stmt,0))!=SQLITE_OK){
			cerr<<"Prepare statement returned error.\n";
			sqlite3_finalize(stmt);
			return -1;
		}
		stepRes = sqlite3_step(stmt);
		if ( !(stepRes==SQLITE_ROW || stepRes==SQLITE_DONE) ){
			cerr<<"Step statement returned error.\n";
			sqlite3_finalize(stmt);
			return -1;
		}
		if (stepRes==SQLITE_DONE){
			cerr<<"No such record.\n";
			sqlite3_finalize(stmt);
			return -1;
		}
		mMBlistSize = sqlite3_column_int(stmt, 0);
		cout<<"mMBlistSize in the query: "<<mMBlistSize<<endl;
		if (mMBlistSize>0)
			mMBlist = new CMB[mMBlistSize];
		sqlite3_finalize(stmt);
		//Fill MBlist
		sprintf(sql,"SELECT  MBno, MBx, MBy, MBw, MBh, MBtype, partMode FROM MBs WHERE FrameID=%d;", frameNo);
		if((sqlite_ret = sqlite3_prepare_v2(db,sql,-1,&stmt,0)) !=SQLITE_OK){
			cerr<<"Prepare statement returned error.\n";
			sqlite3_finalize(stmt);
			return -1;
		}
		int i;
		if (parentVideo->mCodecID == AV_CODEC_ID_H264 || parentVideo->mCodecID == AV_CODEC_ID_MPEG2VIDEO || parentVideo->mCodecID == AV_CODEC_ID_HEVC){ //if it is H264 video

			for (i=0; i<mMBlistSize; i++){
				stepRes = sqlite3_step(stmt);
				if ( !(stepRes==SQLITE_ROW || stepRes==SQLITE_DONE) ){
					cerr<<"Step statement returned error.\n";
					sqlite3_finalize(stmt);
					return -1;
				}
				if (stepRes==SQLITE_DONE){
					cerr<<"No such record.\n";
					//sqlite3_finalize(stmt);
					return -1;
					break;
				}
				mCurrentMB->mMBno =  sqlite3_column_int(stmt, 0);
				mCurrentMB->mMBx =  sqlite3_column_int(stmt, 1);
				mCurrentMB->mMBy =  sqlite3_column_int(stmt, 2);
				mCurrentMB->mMBw =  sqlite3_column_int(stmt, 3);
				mCurrentMB->mMBh =  sqlite3_column_int(stmt, 4);
				mCurrentMB->mMBtype =  sqlite3_column_int(stmt, 5);
				mCurrentMB->mPartMode =  sqlite3_column_int(stmt, 6);
				mCurrentMB->mFrameNo = frameNo;
				mCurrentMB->mSpanRow = mCurrentMB->mMBh/mMinBlockSize;
				mCurrentMB->mSpanColumn = mCurrentMB->mMBw/mMinBlockSize;
				mCurrentMB->db = db;
				mCurrentMB->parentFrame = this;
				if (parentVideo->mCodecID == AV_CODEC_ID_HEVC){
					mCurrentMB->db_fillSubMBListHEVC();
				}
				//mMBlist[(mCurrentMB->mMBy/mMinBlockSize)*mMinBlockWidth + mCurrentMB->mMBx/mMinBlockSize] = (*mCurrentMB);
				mMBlist[i] = (*mCurrentMB);
			}//end for

			if (db_fillSubMBList()){
				//
			}

			mCurrentMB->mSubMBlist = NULL; //They are already copied to the last element of mMBlist[i].
			 //In deconstructor, we wont have problem in this way.
		}// end if codec?= H264
		sqlite3_finalize(stmt);

	}
	return 0;
}
int Cframe::db_fillSubMBList(void){
	char sql[2048];
	int stepRes;
	int sqlite_ret;
	//Fill SubMBlist
	sqlite3_stmt* myStmt; //DONT FORGET TO FINALIZE THIS
	sprintf(sql,"SELECT DISTINCT parentMBID, subMBw, subMBh, subMBx, subMBy FROM MVs WHERE parentFrameID=%d ORDER BY parentMBID;",  frameNo);
	if((sqlite_ret = sqlite3_prepare_v2(db,sql,-1,&myStmt,0))!=SQLITE_OK){
		cerr<<"Prepare statement returned error.\n";
		sqlite3_finalize(myStmt);
		return -1;
	}
	std::map<int, int> NofSubMBsMap; //MBno and NofSubMB
	if(getNofSubMBsList(NofSubMBsMap,myStmt)){
		sqlite3_finalize(myStmt);
		return -1;
	}

	//get MV table
	sqlite3_stmt* myStmtMV; //DONT FORGET TO FINALIZE THIS
	sprintf(sql,"SELECT parentMBID, subMBx, subMBy, MVX, MVY, MVScale, Direction FROM MVs WHERE parentFrameID=%d ORDER BY parentMBID, Direction;", frameNo);
	if((sqlite_ret = sqlite3_prepare_v2(db,sql,-1,&myStmtMV,0))!=SQLITE_OK){
		cerr<<"Prepare statement returned error.\n";
		sqlite3_finalize(myStmtMV);
		return -1;
	}

	//cout<<"[73]: " << NofSubMBsMap[73]<<endl;
	int subMBidx = -1;
	int LastMBid = -1;
	int finished = 0;
	int MBidx = 0;
	if (parentVideo->mCodecID != AV_CODEC_ID_HEVC) {
		while (!finished){
			int stepRes = sqlite3_step(myStmt);
			if ( !(stepRes==SQLITE_ROW || stepRes==SQLITE_DONE) ){
				cerr<<"Step statement returned error.\n";
				sqlite3_finalize(myStmt);
				return -1;
			}
			if (stepRes==SQLITE_DONE){
				cout<<"subMBlist fill is done"<<endl;
				//cerr<<"No such record.\n";
				//TODO: if there is no sub MB, set mNofSubMB to 0, mSubMBlist=NULL.
				//sqlite3_finalize(stmt);
				finished = 1;
				continue;
			}
			int parentMBid = sqlite3_column_int(myStmt, 0);
			int MBw = sqlite3_column_int(myStmt, 1);
			int MBh = sqlite3_column_int(myStmt, 2);
			int MBx = sqlite3_column_int(myStmt, 3);
			int MBy = sqlite3_column_int(myStmt, 4);


			//find corresponding MB from MBlist
			int i;
			for (i=0; i<mMBlistSize; i++){
	//			int currNofSubMB = NofSubMBsMap[mMBlist[i].mMBno];
	//			if (NofSubMB>0)
	//				mMBlist[i].mSubMBlist = new CMB[currNofSubMB];
	//			else
	//				continue;
				if (mMBlist[i].mMBno == parentMBid)
					break;
				else if (i==mMBlistSize-1)//couldnt find
					return -1;
			}
			if (parentMBid == LastMBid)
				subMBidx++;
			else{
				subMBidx = 0;
				LastMBid = parentMBid;
				int currNofSubMB = NofSubMBsMap[mMBlist[i].mMBno];
				if (!mMBlist[i].mSubMBlist)
					mMBlist[i].mSubMBlist = new CMB[currNofSubMB];
				mMBlist[i].mNofSubMB = currNofSubMB;
			}

			mMBlist[i].mSubMBlist[subMBidx].mMBw = MBw;
			mMBlist[i].mSubMBlist[subMBidx].mMBh = MBh;
			mMBlist[i].mSubMBlist[subMBidx].mMBx = MBx;
			mMBlist[i].mSubMBlist[subMBidx].mMBy = MBy;
			mMBlist[i].mSubMBlist[subMBidx].mNofSubMB = 0;
			mMBlist[i].mSubMBlist[subMBidx].parentFrame = this;
			mMBlist[i].mSubMBlist[subMBidx].mSpanRow = MBh/mMinBlockSize;
			mMBlist[i].mSubMBlist[subMBidx].mSpanColumn = MBw/mMinBlockSize;
			mMBlist[i].mSubMBlist[subMBidx].mMBno = mMBlist[i].mMBno;
			mMBlist[i].mSubMBlist[subMBidx].mMBtype = mMBlist[i].mMBtype;
			mMBlist[i].mSubMBlist[subMBidx].mPartMode = mMBlist[i].mPartMode;
			mMBlist[i].mSubMBlist[subMBidx].mFrameNo = frameNo;
			mMBlist[i].mSubMBlist[subMBidx].db = db;
			mMBlist[i].mSubMBlist[subMBidx].mMV = NULL;


		}//End while
	}
	subMBidx = -1;
	LastMBid = -1;
	finished = 0;
	MBidx = 0;
	while (!finished){
		stepRes = sqlite3_step(myStmtMV);
		if ( !(stepRes==SQLITE_ROW || stepRes==SQLITE_DONE) ){
			cerr<<"Step statement returned error.\n";
			sqlite3_finalize(myStmtMV);
			return -1;
		}
		if (stepRes==SQLITE_DONE){
			cout<<"subMVlist fill is done"<<endl;
			//cerr<<"No such record.\n";
			//TODO: if there is no sub MB, set mNofSubMB to 0, mSubMBlist=NULL.
			//sqlite3_finalize(stmt);
			finished = 1;
			continue;
		}
		//parentMBID, subMBx, subMBy, MVX, MVY, MVScale, Direction
		int parentMBid = sqlite3_column_int(myStmtMV, 0);
		int subMBx = sqlite3_column_int(myStmtMV, 1);
		int subMBy = sqlite3_column_int(myStmtMV, 2);

		//Find related MB and its related SubMB
		int i;
		int subMBidx = -1;
		if (mMBlist[MBidx].mMBno != parentMBid || MBidx == 0){ //Search if not the last MB
			for (i=MBidx; i<mMBlistSize; i++){
				if(mMBlist[i].mMBno == parentMBid){
					break;
				}else if (i == mMBlistSize-1){
					cerr<<"Couldnt find MB of this MV."<<endl;
					return -1;
				}

			}
		}
		for (int j=0;j<mMBlist[i].mNofSubMB;j++){
			if( mMBlist[i].mSubMBlist[j].mMBx == subMBx && mMBlist[i].mSubMBlist[j].mMBy == subMBy){
				MBidx = i;
				subMBidx = j;
				break;
			}
		}

		if (subMBidx < 0 ) {//couldnt find SubMB. This is not an error for HEVC.in DB v2
			//cerr<<"MV's subMB couldnt find. Parent MBid: "<<parentMBid<<endl;
			continue;
		}
		//Related MB and subMB are found. Fill MV info
		if (!mMBlist[MBidx].mSubMBlist[subMBidx].mMV){
			mMBlist[MBidx].mSubMBlist[subMBidx].mMV = new CMV;
			mMBlist[MBidx].mSubMBlist[subMBidx].mMV->parentMB = &mMBlist[MBidx];
		}

		int MVx = sqlite3_column_int(myStmtMV, 3);
		int MVy = sqlite3_column_int(myStmtMV, 4);
		int MVScale = sqlite3_column_int(myStmtMV, 5);
		int Direction = sqlite3_column_int(myStmtMV, 6);

		int MVidx=-1;
		switch(Direction) {
		    case -1: MVidx=0; break;
		    case 1 : MVidx=1; break;
		    default: MVidx=-1;
		}
		if (MVidx==-1)
			continue;
		mMBlist[MBidx].mSubMBlist[subMBidx].mMV->MVs[MVidx].x = MVx;
		mMBlist[MBidx].mSubMBlist[subMBidx].mMV->MVs[MVidx].y = MVy;
		mMBlist[MBidx].mSubMBlist[subMBidx].mMV->MVs[MVidx].scale = MVScale;
		mMBlist[MBidx].mSubMBlist[subMBidx].mMV->MVs[MVidx].direction = Direction;

	}
	sqlite3_finalize(myStmt);
	sqlite3_finalize(myStmtMV);
	return 0;
}
int Cframe::getNofSubMBsList(std::map<int, int> &NofSubMBsMap, sqlite3_stmt* myStmt){
	int MBID = -1; int LastMBID = -1;
	int finished = 0;
	if (parentVideo->mCodecID != AV_CODEC_ID_HEVC){
		while (!finished){
			int stepRes = sqlite3_step(myStmt);
			if ( !(stepRes==SQLITE_ROW || stepRes==SQLITE_DONE) ){
				cerr<<"Step statement returned error.\n";
				sqlite3_finalize(myStmt);
				return -1;
			}
			if (stepRes==SQLITE_DONE){
				cout<<"getNofSubMBsList done"<<endl;
				//cerr<<"No such record.\n";
				//TODO: if there is no sub MB, set mNofSubMB to 0, mSubMBlist=NULL.
				//sqlite3_finalize(stmt);
				finished = 1;
				continue;
			}

			MBID = sqlite3_column_int(myStmt, 0);
			if (MBID == LastMBID) {//increment nofSubMB
				NofSubMBsMap[MBID] = NofSubMBsMap[MBID] + 1;
			}
			else{
				NofSubMBsMap[MBID] = 1;
				LastMBID = MBID;
			}


		}
	}
	if (sqlite3_reset(myStmt)){
		cerr<<"A problem occured in resetting stmt."<<endl;
		return -1;
	}
	return 0;
}


int CMB::db_fillSubMBListHEVC(void){// FIXME: SPANS are not correct.
	mNofSubMB = 0;
	int x0,x1,y0,y1;
	int x[4],y[4];
	int spanRow[4], spanColumn[4];
    int MBw[4], MBh[4];
	if (mPartMode==PART_NxN){//Divide into four equal parts
		mNofSubMB = 4;
		x0 = mMBx;
		y0 = mMBy;
		x1 = mMBx + mMBw/2;
		y1 = mMBy + mMBh/2;
		x[0] = x0; x[1] = x1; x[2] = x0; x[3] = x1;
		y[0] = y0; y[1] = y0; y[2] = y1; y[3] = y1;
		spanRow[0] = spanRow[1] = spanRow[2] = spanRow[3] = mMBh/2;
		spanColumn[0] = spanColumn[1] = spanColumn[2] = spanColumn[3] = mMBw/2;
        MBw[0] = MBw[1] = MBw[2] = MBw[3] = mMBw/2;
        MBh[0] = MBh[1] = MBh[2] = MBh[3] = mMBh/2;
	}
	if (mPartMode==PART_2Nx2N){//Divide into four equal parts
		mNofSubMB = 1;
		x0 = mMBx;
		y0 = mMBy;
		x[0] = x[1] = x[2] = x[3] = x0;
		y[0] = y[1] = y[2] = y[3] = y0;
		spanRow[0] = spanRow[1] = spanRow[2] = spanRow[3] = mMBh;
		spanColumn[0] = spanColumn[1] = spanColumn[2] = spanColumn[3] = mMBw;
        MBw[0] = MBw[1] = MBw[2] = MBw[3] = mMBw;
        MBh[0] = MBh[1] = MBh[2] = MBh[3] = mMBh;
	}
	mSubMBlist = new CMB[mNofSubMB];
	//Fill SubMBlist

	for(int i=0; i<mNofSubMB; i++){

		//mSubMBlist[i] = *this;
		mSubMBlist[i].mNofSubMB = 0;
        mSubMBlist[i].mMBw = MBw[i];
        mSubMBlist[i].mMBh = MBh[i];
		mSubMBlist[i].mMBx = x[i];
		mSubMBlist[i].mMBy = y[i];
		mSubMBlist[i].parentFrame = parentFrame;
		mSubMBlist[i].mSpanRow = spanRow[i];
		mSubMBlist[i].mSpanColumn = spanColumn[i];
		mSubMBlist[i].mMBno = mMBno;
		mSubMBlist[i].mMBtype = mMBtype;
		mSubMBlist[i].mPartMode = mPartMode;
		mSubMBlist[i].mFrameNo = mFrameNo;
		mSubMBlist[i].db = db;
		//Fill MVs of that SubMB
//		if (mSubMBlist[i].fillMVs()){
//			return -1;
//		}
	}


	return mNofSubMB;
}

int CMB::fillMVs(void){
	char sql[2048];
	int stepRes;
	int sqlite_ret;
	sprintf(sql,"SELECT MVX, MVY, MVScale, Direction FROM MVs WHERE parentMBID=%d AND parentFrameID=%d AND subMBx=%d AND subMBy=%d ORDER BY Direction;", mMBno, mFrameNo, mMBx, mMBy);
	if((sqlite_ret = sqlite3_prepare_v2(db,sql,-1,&stmt,0))!=SQLITE_OK){
		cerr<<"Prepare statement returned error.\n";
		sqlite3_finalize(stmt);
		return -1;
	}
	for (int i=0; i<2; i++){
		stepRes = sqlite3_step(stmt);
		if ( !(stepRes==SQLITE_ROW || stepRes==SQLITE_DONE) ){
			cerr<<"Step statement returned error.\n";
			sqlite3_finalize(stmt);
			return -1;
		}
		if (stepRes==SQLITE_DONE){
			//this is not an error//cerr<<"No such record.\n";
			sqlite3_finalize(stmt);
			return 0;
		}
		if (!mMV){
			mMV = new CMV;
			mMV->parentMB = this;
		}
		mMV->MVs[i].x = sqlite3_column_int(stmt, 0);
		mMV->MVs[i].y = sqlite3_column_int(stmt, 1);
		mMV->MVs[i].scale = sqlite3_column_int(stmt, 2);
		mMV->MVs[i].direction = sqlite3_column_int(stmt, 3);
	}
	sqlite3_finalize(stmt);
	return 0;
}

void Cframe::setStateReady(bool state){
	mReady = state;
	if (mCurrentMB)
		mCurrentMB->setStateReady(state);
}

AVPictureType Cframe::getPictureType(void){
    char sql[2048];
    int stepRes;
    AVPictureType pictType;
	if (mReady){
		sprintf(sql,"SELECT PictureType FROM FRAMEs WHERE CodedPictureNumber=%d;",frameNo);
		if(sqlite3_prepare_v2(db,sql,-1,&stmt,0)!=SQLITE_OK){
			cerr<<"Prepare statement returned error.\n";
			setStateReady(false);
			sqlite3_finalize(stmt);
			return AV_PICTURE_TYPE_NONE;
		}
		stepRes = sqlite3_step(stmt);
		if ( !(stepRes==SQLITE_ROW || stepRes==SQLITE_DONE) ){
			cerr<<"Step statement returned error.\n";
			setStateReady(false);
			sqlite3_finalize(stmt);
			return AV_PICTURE_TYPE_NONE;
		}
		if (stepRes==SQLITE_DONE){
			cerr<<"No such record.\n";
			sqlite3_finalize(stmt);
			setStateReady(false);
			return AV_PICTURE_TYPE_NONE;
		}

		pictType = (AVPictureType) sqlite3_column_int(stmt, 0);
		setStateReady(true);
		sqlite3_finalize(stmt);
		return pictType;
	}
	else{
		cout<<"Frame is not ready.";
		setStateReady(false);
		return AV_PICTURE_TYPE_NONE;
	}
	return AV_PICTURE_TYPE_NONE;
}

string Cframe::getPictureTypeStr(void){
	string ret="";
	switch(getPictureType())
	{
	    case AV_PICTURE_TYPE_NONE  : ret="None";   break;
	    case AV_PICTURE_TYPE_I: ret="I"; break;
	    case AV_PICTURE_TYPE_P : ret="P";  break;
	    case AV_PICTURE_TYPE_B : ret="B";  break;
	    case AV_PICTURE_TYPE_S  : ret="S";   break;
	    case AV_PICTURE_TYPE_SI: ret="SI"; break;
	    case AV_PICTURE_TYPE_SP : ret="SP";  break;
	    case AV_PICTURE_TYPE_BI : ret="BI";  break;
	}
	return ret;
}

CMB* Cframe::MB(int mbx,int mby){

	return mCurrentMB;
}

///////Macroblock
CMB::CMB(){
	mReady = false;
	mMBx = -1;
	mMBy = -1;
	mMBw = -1;
	mMBh = -1;
	mMBno = -1;
	mMBtype = -1;
	mPartMode = -1;
	mFrameNo = -1;
	mMV = NULL;
	db=NULL;
	mSpanRow = -1;
	mSpanColumn = -1;
	mSubMBlist = NULL;
	mNofSubMB = -1;
	mMV = NULL; //new CMV();
	stmt = NULL;
	parentFrame = NULL;
	//mMV->parentMB = this;
}
CMB::~CMB(){
	if (mMV){
		delete mMV;
		mMV = NULL;
	}
	if (mSubMBlist){
		delete[] mSubMBlist;
		mSubMBlist = NULL;
	}
}

CMB &CMB::operator=(const CMB &D )
{
	mMBno =  D.mMBno;
	mMBx =  D.mMBx;
	mMBy =  D.mMBy;
	mMBw =  D.mMBw;
	mMBh =  D.mMBh;
	mMBtype =  D.mMBtype;
	mPartMode =  D.mPartMode ;
	mFrameNo = D.mFrameNo;
	db = D.db;
	mMBno = D.mMBno;
	mMV = D.mMV; //TODO:
	mSpanRow = D.mSpanRow;
	mSpanColumn = D.mSpanColumn;
	mSubMBlist = D.mSubMBlist;
	mNofSubMB = D.mNofSubMB;
	parentFrame = D.parentFrame;
	return *this;
}

void CMB::setStateReady(bool state){
	mReady = state;
	if(mMV)
		mMV->setStateReady(state);
}

int32_t CMB::getMBType(){
    int32_t mbType=-1;

	return mbType;
}
string CMB::getMBTypeStr(){
/*	int32_t mbt = getMBType();
	if (mbt<0){
		cerr<<"MB type error."<<endl;
		return "";
	}
	string str_mbType = string((IS_16X16(mbt))?"16x16 ":"") \
			+ string((IS_DIRECT(mbt))?"Direct ":"") \
			+ string((IS_8X8(mbt))?"8x8 ":"");
	return str_mbType;*/
	return "";
}


///////Motion vector
CMV::CMV(){
	stmt = NULL;
	db = NULL;
	mReady = false;
	parentMB = NULL;
	MVs[0].x = MVs[1].x = MVs[0].y = MVs[1].y = MVs[0].direction = MVs[1].direction = MVs[0].scale = MVs[1].scale = 0;
}
CMV::~CMV(){
}

void CMV::setStateReady(bool state){
	mReady = state;
}

SMV CMV::getMVvalues(void){
	SMV ret;

	return ret;
}
void CMV::printInfo(){
	cout<<"info test\n";
}

} /* namespace vcal */
