/*
 * vcal_test.cpp
 *
 *  Created on: Jun 7, 2016
 *      Author: kkk
 */

#include "vcal.h"
using namespace std;
int main(void) {
	vcal::Cvideo *videoInfo = new vcal::Cvideo("/home/kkk/Desktop/videos/HEVC.db");
	//dnm->frame->MB->MV->printInfo();
	//dnm->open("/home/kkk/Desktop/extractMVsvn/mpeg2.db");
	//std::cout<<dnm->pathToVideoInfoDB;
	//dnm->getFrame(2);
	//std::cout<<videoInfo->frame(5)->getPictureTypeStr()<< std::endl;
	//std::cout<<videoInfo->frame(5)->MB(18,0)->getMBTypeStr()<< std::endl;
	//std::cout<<"Number of submacroblocks: "<<videoInfo->frame(3)->MB(16,4)->getNofSubMBs()<<std::endl;
//	std::cout<<"test "<<123<<" test"<<std::endl;
//	int nofSubBlocks = videoInfo->frame(3)->MB(16,4)->getNofSubMBs();
//	nofSubBlocks = videoInfo->frame(3)->MB(16,5)->getNofSubMBs();
//	vcal::SMV mv;
//	for (int i=0; i < nofSubBlocks; i++){
//		mv = videoInfo->frame(3)->MB(16,4)->MV(i)->getMVvalues();
//		std::cout<<mv.x;
//	}
	int frameNo=3;
	cout<<"Fetching frame "<<frameNo<<endl;
	videoInfo->frame(frameNo);
	cout<<"Frame "<<frameNo<<" is ready." <<endl;
	frameNo=4;
	cout<<"Fetching frame "<<frameNo<<endl;
	videoInfo->frame(frameNo);
	cout<<"Frame "<<frameNo<<" is ready." <<endl;

	vcal::CMB* myMBlist = videoInfo->frame(frameNo)->mMBlist;
	for (int i=0; i < videoInfo->frame(frameNo)->mMBlistSize; i++){
		cout<<"i:"<<i<<", x: "<< myMBlist[i].mMBx <<" y:"<<myMBlist[i].mMBy <<" Span: "<<myMBlist[i].mSpanColumn <<" ,"<<myMBlist[i].mSpanRow<<" NofSubMB: "<<myMBlist[i].mNofSubMB<<endl;
		for (int j=0;j<myMBlist[i].mNofSubMB;j++){
			cout<<" Sub"<<j<<" x: "<<myMBlist[i].mSubMBlist[j].mMBx<<" y: "<<myMBlist[i].mSubMBlist[j].mMBy<< " Span: "<<myMBlist[i].mSubMBlist[j].mSpanColumn <<" ,"<<myMBlist[i].mSubMBlist[j].mSpanRow <<
					" MV: x:"<<myMBlist[i].mSubMBlist[j].mMV->MVs[0].x
					<<" y:"<<myMBlist[i].mSubMBlist[j].mMV->MVs[0].y
					<<" dir:"<<myMBlist[i].mSubMBlist[j].mMV->MVs[0].direction
					<<" MV: x:"<<myMBlist[i].mSubMBlist[j].mMV->MVs[1].x
					<<" y:"<<myMBlist[i].mSubMBlist[j].mMV->MVs[1].y
					<<" dir:"<<myMBlist[i].mSubMBlist[j].mMV->MVs[1].direction
					<<endl;
		}
		cout<<endl;
	}
	delete videoInfo;
	return 0;
}
