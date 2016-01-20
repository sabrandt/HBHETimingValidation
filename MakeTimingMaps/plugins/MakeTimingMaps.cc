// -*- C++ -*-
//
// Package:    HBHETimingValidation/MakeTimingMaps
// Class:      MakeTimingMaps
// 
/**\class MakeTimingMaps MakeTimingMaps.cc HBHETimingValidation/MakeTimingMaps/plugins/MakeTimingMaps.cc

 Description: Quick example code for making HBHE timing monitoring plots
*/
//
// Original Author:  Stephanie Brandt
//         Created:  Fri, 21 Aug 2015 11:42:17 GMT
//
//


// system include files
#include <memory>
#include <string>
#include <map>
#include <iostream>
using namespace std;

// user include files
#include "TTree.h"
#include "TFile.h"
#include "TProfile.h"
#include "TH1.h"
#include "TH2.h"
#include "TCanvas.h"
#include "TProfile2D.h"

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/Framework/interface/Event.h"

#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "DataFormats/HcalRecHit/interface/HBHERecHit.h"
#include "DataFormats/HcalRecHit/interface/HcalRecHitCollections.h"
#include "DataFormats/HcalDetId/interface/HcalDetId.h"

#include "SimDataFormats/CaloHit/interface/PCaloHit.h"
#include "SimDataFormats/CaloHit/interface/PCaloHitContainer.h"

#include "SimCalorimetry/HcalSimAlgos/interface/HcalSimParameterMap.h"
//
// class declaration
//

// If the analyzer does not use TFileService, please remove
// the template argument to the base class so the class inherits
// from  edm::one::EDAnalyzer<> and also remove the line from
// constructor "usesResource("TFileService");"
// This will improve performance in multithreaded jobs.

class MakeTimingMaps : public edm::one::EDAnalyzer<edm::one::SharedResources>  {
   public:
      explicit MakeTimingMaps(const edm::ParameterSet&);
      ~MakeTimingMaps();

      static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);


   private:
      virtual void beginJob() override;
      virtual void analyze(const edm::Event&, const edm::EventSetup&) override;
      virtual void endJob() override;
      
      std::string int2string(int i);
      
      void ClearVariables();
      
      // some variables for storing information
      double Method0Energy;
      double RecHitEnergy;
      double RecHitTime;
      double iEta;
      double iPhi; 
      int depth;
      int RunNumber;
      int EvtNumber;
      
      // create the output file
      edm::Service<TFileService> FileService;
      // create the token to retrieve hit information
      edm::EDGetTokenT<HBHERecHitCollection>    hRhToken;
      
      // 2-d profiles which will hold the average time per channel, 1 for each depth
      TProfile2D *hHBHETiming_Depth1;
      TProfile2D *hHBHETiming_Depth2;
      TProfile2D *hHBHETiming_Depth3;
      
      TH2F *occupancy_d1;
      TH2F *occupancy_d2;
      TH2F *occupancy_d3;
      
      // pointer to pointer to pointers to make a 2-d array of histograms
      // These are used to hold the individual rechit timing histograms for each channel
      TH1F ***hTiming_Depth1 = new TH1F**[59];
      TH1F ***hTiming_Depth2 = new TH1F**[59];
      TH1F ***hTiming_Depth3 = new TH1F**[6];
      
      // Sample histogram for storing time slice information
      TH1F *hTimeSlices;
      
      int runNumber_;
};

MakeTimingMaps::MakeTimingMaps(const edm::ParameterSet& iConfig)
{
   //now do what ever initialization is needed
   usesResource("TFileService");
    
  for (int i = 0; i < 59; ++i){
    hTiming_Depth1[i] = new TH1F*[72];
    hTiming_Depth2[i] = new TH1F*[72];
  }
  for(int i = 0; i < 6; ++i){
    hTiming_Depth3[i] = new TH1F*[72]; 
  }
  
  // Tell which collection is consumed
  hRhToken = consumes<HBHERecHitCollection >(iConfig.getUntrackedParameter<string>("HBHERecHits","hbheprereco"));
  
  // There is a configurable parameter called "runNumber" in ConfFile_cfg.py
  // This line gets the (integer-value) parameter which is specified there and 
  // assigns this value to runNumber_
  runNumber_ = iConfig.getParameter<int>("runNumber");
  
  // book all the 1-d histograms which store the rechit/channel information
  // this is done is a really ridiculous way and needs to be replaced
  for(int i = 0; i < 72; ++i){
    for(int j = 0; j < 59; ++j){
       hTiming_Depth1[j][i] = FileService->make<TH1F>(("Depth1_ieta"+int2string(j-29)+"_iphi"+int2string(i+1)).c_str(),("Depth1_ieta"+int2string(j-29)+"_iphi"+int2string(i+1)).c_str(),75,-37.5,37.5);
       hTiming_Depth2[j][i] = FileService->make<TH1F>(("Depth2_ieta"+int2string(j-29)+"_iphi"+int2string(i+1)).c_str(),("Depth2_ieta"+int2string(j-29)+"_iphi"+int2string(i+1)).c_str(),75,-37.5,37.5);
    } 
    for(int j = 0; j < 6; ++j){ 
      // Special thing for Depth #3 because there are only a few strips of channels
      int eta = 0;
      if(j==0) eta = -28;
      if(j==1) eta = -27;
      if(j==2) eta = -16;
      if(j==3) eta =  16;
      if(j==4) eta =  27;
      if(j==5) eta =  28;
      hTiming_Depth3[j][i] = FileService->make<TH1F>(("Depth3_ieta"+int2string(eta)+"_iphi"+int2string(i+1)).c_str(),("Depth3_ieta"+int2string(eta)+"_iphi"+int2string(i+1)).c_str(),75,-37.5,37.5);
    }
  }
  
  // example histogram for storing time slice information
  hTimeSlices = FileService->make<TH1F>("hTimeSlices","hTimeSlices",10,-100,150); 

  // Make all the 2-d profiles 
  // could have been done better
  // consider replacing the 2-d profiles and the per-channel 1-d rechit timing distributions with a 3-d histogram (maybe)
  // also consider writing these as arrays
  // e.g. declare the histogram array above as 
  // TH3D **hHBHETiming = new TH3D*[3];
  // then here make a loop over the three depths and do
  // hHBHETiming[i] = FileService->make<TH3D>(("timing_depth"+int2string(i)).c_str(),("timing_depth"+int2string(i)).c_str(),59,-29.5,29.5,72,0.5,72.5, 75,-37.5, 37.5);
  
  hHBHETiming_Depth1 = FileService->make<TProfile2D>("hHBHETiming_Depth1","hHBHETiming_Depth1",59,-29.5,29.5,72,0.5,72.5, -12.5, 12.5,"s");
  hHBHETiming_Depth2 = FileService->make<TProfile2D>("hHBHETiming_Depth2","hHBHETiming_Depth2",59,-29.5,29.5,72,0.5,72.5, -12.5, 12.5,"s");
  hHBHETiming_Depth3 = FileService->make<TProfile2D>("hHBHETiming_Depth3","hHBHETiming_Depth3",59,-29.5,29.5,72,0.5,72.5, -12.5, 12.5,"s");
  
  occupancy_d1 = FileService->make<TH2F>("occupancy_d1","occupancy_depth1",59,-29.5,29.5,72,0.5,72.5);
  occupancy_d2 = FileService->make<TH2F>("occupancy_d2","occupancy_depth2",59,-29.5,29.5,72,0.5,72.5);
  occupancy_d3 = FileService->make<TH2F>("occupancy_d3","occupancy_depth3",59,-29.5,29.5,72,0.5,72.5);
}


MakeTimingMaps::~MakeTimingMaps(){} // destructor

// funtion so you can turn an integer into a string for labels and stuff
std::string MakeTimingMaps::int2string(int i) {
  stringstream ss;
  string ret;
  ss << i;
  ss >> ret;
  return ret;
}

// ------------ method called for each event  ------------
void
MakeTimingMaps::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup)
{
  using namespace edm;

  // Read events
  Handle<HBHERecHitCollection> hRecHits; // create handle
  iEvent.getByToken(hRhToken, hRecHits); // get events based on token
   
   // get the run number of the event 
  RunNumber = iEvent.id().run();
  
  // Loop over all rechits in one event
  for(int i = 0; i < (int)hRecHits->size(); i++) {
    ClearVariables(); // sets a bunch of stuff to zero
    
    RunNumber = iEvent.id().run(); // get the run number for the event
    EvtNumber = iEvent.id().event(); // get the event number

    // Just in case the file you are running over contains events from multiple runs,
    // remove everything except the run you are interested in
    if(RunNumber != runNumber_) continue;
    
    // get ID information for the reconstructed hit
    HcalDetId detID_rh = (*hRecHits)[i].id().rawId();
    
    // ID information can get us detector coordinates
    depth = (*hRecHits)[i].id().depth();
    iEta = detID_rh.ieta();
    iPhi = detID_rh.iphi();
    
    // get some variables
    Method0Energy = (*hRecHits)[i].eraw();
    RecHitEnergy = (*hRecHits)[i].energy();
    RecHitTime = (*hRecHits)[i].time();


    // this block of code is to extract the charge in individual time slices from
    // the auxiliary information
    // Not  used at the moment, but useful for trouble-shooting if we identify a 
    //  problem with some channels
    int adc[8];
    int auxwd1 = (*hRecHits)[i].auxHBHE();  // TS = 0,1,2,3 info
    int auxwd2 = (*hRecHits)[i].aux();      // TS = 4,5,6,7 info

    adc[0] = (auxwd1)       & 0x7F;
    adc[1] = (auxwd1 >> 7)  & 0x7F;
    adc[2] = (auxwd1 >> 14) & 0x7F;
    adc[3] = (auxwd1 >> 21) & 0x7F;

    adc[4] = (auxwd2)       & 0x7F;
    adc[5] = (auxwd2 >> 7)  & 0x7F;
    adc[6] = (auxwd2 >> 14) & 0x7F;
    adc[7] = (auxwd2 >> 21) & 0x7F;
    
    // example on how to fill the time slices if you need it
    for(int nadc = 0; nadc < 8; nadc++){
      hTimeSlices->SetBinContent(nadc+1,adc[nadc]);
    }
    
    // only get timing information from rechits with high enough energy
    if(RecHitEnergy > 5.0) {
      
      if(depth==1){// fill depth1
        hHBHETiming_Depth1->Fill(detID_rh.ieta(), detID_rh.iphi(), RecHitTime);
        occupancy_d1->Fill(detID_rh.ieta(), detID_rh.iphi(),1);
        hTiming_Depth1[detID_rh.ieta()+29][detID_rh.iphi()-1]->Fill(RecHitTime);
      } else if(depth==2){// fill depth 2
        hHBHETiming_Depth2->Fill(detID_rh.ieta(), detID_rh.iphi(), RecHitTime);
        occupancy_d2->Fill(detID_rh.ieta(), detID_rh.iphi(),1);
        hTiming_Depth2[detID_rh.ieta()+29][detID_rh.iphi()-1]->Fill(RecHitTime);
      } else if(depth==3){
        hHBHETiming_Depth3->Fill(detID_rh.ieta(), detID_rh.iphi(), RecHitTime);
        occupancy_d3->Fill(detID_rh.ieta(), detID_rh.iphi(),1);
        
        int bin = 0;
        if(detID_rh.ieta() == -28) bin = 0;
        if(detID_rh.ieta() == -27) bin = 1;
        if(detID_rh.ieta() == -16) bin = 2;
        if(detID_rh.ieta() ==  16) bin = 3;
        if(detID_rh.ieta() ==  27) bin = 4;
        if(detID_rh.ieta() ==  28) bin = 5;
        hTiming_Depth3[bin][detID_rh.iphi()-1]->Fill(RecHitTime);
      }
    }
  }
}


// ------------ method called once each job just before starting event loop  ------------
void MakeTimingMaps::beginJob(){}

// ------------ method called once each job just after ending the event loop  ------------
void MakeTimingMaps::endJob(){}

void MakeTimingMaps::ClearVariables(){
 RecHitEnergy = 0;
 RunNumber = 0;
 depth=0;
 iEta = 0;
 iPhi = 0;
 RecHitTime = 0;
}

// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void MakeTimingMaps::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  // The following says we do not know what parameters are allowed so do no validation
  // Please change this to state exactly what you do use, even if it is no parameters
  edm::ParameterSetDescription desc;
  desc.setUnknown();
  descriptions.addDefault(desc);
}

//define this as a plug-in
DEFINE_FWK_MODULE(MakeTimingMaps);
