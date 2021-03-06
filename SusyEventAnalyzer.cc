#define SusyEventAnalyzer_cxx

#include <TH2.h>
#include <TH3.h>
#include <TStyle.h>
#include <TCanvas.h>
#include <TH1F.h>
#include <TRandom3.h>
#include <TObject.h>

#include <map>
#include <set>
#include <cmath>
#include <algorithm>
#include <utility>

#include "SusyEventAnalyzer.h"
#include "BtagWeight.h"
#include "EventQuality.h"

using namespace std;

bool sortTriggers(pair<TString, int> i, pair<TString, int> j) { return (i.second > j.second); }

void SusyEventAnalyzer::PileupWeights(TString puFile, TString puFile_up, TString puFile_down) {

  TFile * in = new TFile(puFile, "READ");
  TFile * in_up = new TFile(puFile_up, "READ");
  TFile * in_down = new TFile(puFile_down, "READ");

  TH1F * _data = (TH1F*)in->Get("pileup");
  TH1F * _data_up = (TH1F*)in_up->Get("pileup");
  TH1F * _data_down = (TH1F*)in_down->Get("pileup");
  
  TString output_code_t = FormatName(scan);

  TH1F * data = (TH1F*)_data->Clone("pu_data"+output_code_t); data->Sumw2();
  TH1F * data_up = (TH1F*)_data_up->Clone("pu_data_up"+output_code_t); data_up->Sumw2();
  TH1F * data_down = (TH1F*)_data_down->Clone("pu_data_down"+output_code_t); data_down->Sumw2();

  TH1F * mc = new TH1F("pu_mc"+output_code_t, "pu_mc"+output_code_t, 70, 0, 70); mc->Sumw2();

  Long64_t nEntries = fTree->GetEntries();
  cout << "Total events in files : " << nEntries << endl;
  cout << "Events to be processed : " << processNEvents << endl;

  Long64_t jentry = 0;
  while(jentry != processNEvents && event.getEntry(jentry++) != 0) {

    int nPV = -1;
    susy::PUSummaryInfoCollection::const_iterator iBX = event.pu.begin();
    bool foundInTimeBX = false;
    while((iBX != event.pu.end()) && !foundInTimeBX) {
      if(iBX->BX == 0) {
	nPV = iBX->trueNumInteractions;
	foundInTimeBX = true;
      }
      ++iBX;
    }
    
    if(foundInTimeBX) mc->Fill(nPV);

  } // end event loop

  TH1D * data_nonorm = (TH1D*)data->Clone("pu_data_nonorm"+output_code_t);
  TH1D * data_up_nonorm = (TH1D*)data_up->Clone("pu_data_up_nonorm"+output_code_t);
  TH1D * data_down_nonorm = (TH1D*)data_down->Clone("pu_data_down_nonorm"+output_code_t);
  TH1D * mc_nonorm = (TH1D*)mc->Clone("pu_mc_nonorm"+output_code_t);

  Double_t intData = data->Integral();
  Double_t intData_up = data_up->Integral();
  Double_t intData_down = data_down->Integral();
  Double_t intMC = mc->Integral();

  data->Scale(1./intData);
  data_up->Scale(1./intData_up);
  data_down->Scale(1./intData_down);
  mc->Scale(1./intMC);

  TH1F * weights = (TH1F*)data->Clone("puWeights"+output_code_t);
  weights->Divide(mc);

  TH1F * weights_up = (TH1F*)data_up->Clone("puWeights_up"+output_code_t);
  weights_up->Divide(mc);

  TH1F * weights_down = (TH1F*)data_down->Clone("puWeights_down"+output_code_t);
  weights_down->Divide(mc);

  TFile * out = new TFile("pileupReweighting"+output_code_t+".root", "RECREATE");
  out->cd();

  data->Write();
  data_nonorm->Write();

  data_up->Write();
  data_up_nonorm->Write();

  data_down->Write();
  data_down_nonorm->Write();

  mc->Write();
  mc_nonorm->Write();

  weights->Write();
  weights_up->Write();
  weights_down->Write();

  out->Write();
  out->Close();

  in->Close();
  in_up->Close();
  in_down->Close();

  return;
}

void SusyEventAnalyzer::CalculateBtagEfficiency() {

  const int NCNT = 50;
  int nCnt[NCNT][nChannels];
  for(int i = 0; i < NCNT; i++) {
    for(int j = 0; j < nChannels; j++) {
      nCnt[i][j] = 0;
    }
  }
  
  TString output_code_t = FormatName(scan);

  // open histogram file and define histograms
  TFile * out = new TFile("btagEfficiency"+output_code_t+".root", "RECREATE");
  out->cd();

  TH1F * num_bjets = new TH1F("bjets"+output_code_t, "bjets"+output_code_t, 200, 0, 1000); num_bjets->Sumw2();
  TH1F * num_btags = new TH1F("btags"+output_code_t, "btags"+output_code_t, 200, 0, 1000); num_btags->Sumw2();
  TH1F * num_cjets = new TH1F("cjets"+output_code_t, "cjets"+output_code_t, 200, 0, 1000); num_cjets->Sumw2();
  TH1F * num_ctags = new TH1F("ctags"+output_code_t, "ctags"+output_code_t, 200, 0, 1000); num_ctags->Sumw2();
  TH1F * num_ljets = new TH1F("ljets"+output_code_t, "ljets"+output_code_t, 200, 0, 1000); num_ljets->Sumw2();
  TH1F * num_ltags = new TH1F("ltags"+output_code_t, "ltags"+output_code_t, 200, 0, 1000); num_ltags->Sumw2();

  ScaleFactorInfo sf(btagger);

  Long64_t nEntries = fTree->GetEntries();
  cout << "Total events in files : " << nEntries << endl;
  cout << "Events to be processed : " << processNEvents << endl;

  vector<susy::PFJet*> pfJets, btags;
  vector<TLorentzVector> pfJets_corrP4, btags_corrP4;
  vector<float> csvValues;
  vector<susy::Muon*> tightMuons, looseMuons;
  vector<susy::Electron*> tightEles, looseEles;
  vector<susy::Photon*> photons;
  vector<BtagInfo> tagInfos;

  // start event looping
  Long64_t jentry = 0;
  while(jentry != processNEvents && event.getEntry(jentry++) != 0) {

    if(printLevel > 0 || (printInterval > 0 && (jentry >= printInterval && jentry%printInterval == 0))) {
      cout << int(jentry) << " events processed with run = " << event.runNumber << ", event = " << event.eventNumber << endl;
    }

    nCnt[0][0]++; // events

    int nPVertex = GetNumberPV(event);
    if(nPVertex == 0) continue;

    float HT = 0.;

    tightMuons.clear();
    looseMuons.clear();
    tightEles.clear();
    looseEles.clear();
    photons.clear();
    pfJets.clear();
    btags.clear();
    pfJets_corrP4.clear();
    btags_corrP4.clear();
    csvValues.clear();
    tagInfos.clear();

    findMuons(event, tightMuons, looseMuons, HT, kSignal);
    if(tightMuons.size() > 1 || looseMuons.size() > 0) continue;

    findElectrons(event, tightMuons, looseMuons, tightEles, looseEles, HT, kSignal);
    if(tightEles.size() > 1 || looseEles.size() > 0) continue;

    if(tightMuons.size() + tightEles.size() != 1) continue;
    if(looseMuons.size() + looseEles.size() != 0) continue;

    bool passHLT = true;
    if(useTrigger) {
      if(tightEles.size() == 1) passHLT = PassTriggers(1);
      else if(tightMuons.size() == 1) passHLT = PassTriggers(2);
    }
    if(!passHLT) continue;

    findPhotons(event, 
		photons,
		tightMuons, looseMuons,
		tightEles, looseEles,
		HT,
		kSignalPhotons);

    TLorentzVector hadronicSystem(0., 0., 0., 0.);

    findJets(event, 
	     tightMuons, looseMuons,
	     tightEles, looseEles,
	     photons,
	     pfJets, btags,
	     sf,
	     tagInfos, csvValues, 
	     pfJets_corrP4, btags_corrP4, 
	     HT, hadronicSystem);

    ////////////////////

    for(unsigned int iJet = 0; iJet < pfJets.size(); iJet++) {
      map<TString, Float_t>::iterator s_it = pfJets[iJet]->jecScaleFactors.find("L1FastL2L3");
      if(s_it == pfJets[iJet]->jecScaleFactors.end()) {
	continue;
      }
      float scale = s_it->second;
      TLorentzVector corrP4 = scale * pfJets[iJet]->momentum;
      if(fabs(corrP4.Eta()) >= 2.4) continue;
	  
      if(fabs(pfJets[iJet]->algDefFlavour) == 5) {
	num_bjets->Fill(corrP4.Pt());
	if((btagger == "CSVL" && pfJets[iJet]->bTagDiscriminators[susy::kCSV] > 0.244) ||
	   (btagger == "CSVM" && pfJets[iJet]->bTagDiscriminators[susy::kCSV] > 0.679) ||
	   (btagger == "CSVT" && pfJets[iJet]->bTagDiscriminators[susy::kCSV] > 0.898)) 
	  num_btags->Fill(corrP4.Pt());
      }
	  
      if(fabs(pfJets[iJet]->algDefFlavour) == 4) {
	num_cjets->Fill(corrP4.Pt());
	if((btagger == "CSVL" && pfJets[iJet]->bTagDiscriminators[susy::kCSV] > 0.244) ||
	   (btagger == "CSVM" && pfJets[iJet]->bTagDiscriminators[susy::kCSV] > 0.679) ||
	   (btagger == "CSVT" && pfJets[iJet]->bTagDiscriminators[susy::kCSV] > 0.898)) 
	  num_ctags->Fill(corrP4.Pt());
      }
	  
      if(fabs(pfJets[iJet]->algDefFlavour) == 1 ||
	 fabs(pfJets[iJet]->algDefFlavour) == 2 ||
	 fabs(pfJets[iJet]->algDefFlavour) == 3 ||
	 fabs(pfJets[iJet]->algDefFlavour) == 21) {
	num_ljets->Fill(corrP4.Pt());
	if((btagger == "CSVL" && pfJets[iJet]->bTagDiscriminators[susy::kCSV] > 0.244) ||
	   (btagger == "CSVM" && pfJets[iJet]->bTagDiscriminators[susy::kCSV] > 0.679) ||
	   (btagger == "CSVT" && pfJets[iJet]->bTagDiscriminators[susy::kCSV] > 0.898)) 
	  num_ltags->Fill(corrP4.Pt());
      }
	  
    } // for jets
	
  } // for entries

  TH1F * bEff = (TH1F*)num_btags->Clone("bEff"+output_code_t);
  bEff->Divide(num_bjets);

  TH1F * cEff = (TH1F*)num_ctags->Clone("cEff"+output_code_t);
  cEff->Divide(num_cjets);

  TH1F * lEff = (TH1F*)num_ltags->Clone("lEff"+output_code_t);
  lEff->Divide(num_ljets);

  out->Write();
  out->Close();

}

void SusyEventAnalyzer::Data() {

  TFile* out = new TFile("hist_"+outputName+"_"+btagger+".root", "RECREATE");
  out->cd();

  const int NCNT = 50;
  int nCnt[NCNT][nChannels];
  for(int i = 0; i < NCNT; i++) {
    for(int j = 0; j < nChannels; j++) {
      nCnt[i][j] = 0;
    }
  }

  ///////////////////////////////////////////////////
  // Define histograms to be filled for all events
  ///////////////////////////////////////////////////

  TString metFilterNames[susy::nMetFilters] = {
    "CSCBeamHalo",
    "HcalNoise",
    "EcalDeadCellTP",
    "EcalDeadCellBE",
    "TrackingFailure",
    "EEBadSC",
    "HcalLaserOccupancy",
    "HcalLaserEventList",
    "HcalLaserRECOUserStep",
    "EcalLaserCorr",
    "ManyStripClus53X",
    "TooManyStripClus53X",
    "LogErrorTooManyClusters",
    "LogErrorTooManyTripletsPairs",
    "LogErrorTooManySeeds",
    "EERingOfFire",
    "InconsistentMuon",
    "GreedyMuon"};

  TH2F* h_metFilter = new TH2F("metFilter", "MET Filter Failures", susy::nMetFilters, 0, susy::nMetFilters, susy::nMetFilters, 0, susy::nMetFilters);
  for(int i = 0; i < susy::nMetFilters; i++) {
    h_metFilter->GetXaxis()->SetBinLabel(i+1, metFilterNames[i]);
    h_metFilter->GetYaxis()->SetBinLabel(i+1, metFilterNames[i]);
  }

  /////////////////////////////////
  // Reweighting trees
  /////////////////////////////////

  const int nTreeVariables = 115;

  TString varNames[nTreeVariables] = {
    "pfMET", "pfMET_x", "pfMET_y", "pfMET_phi",
    "pfMET_sysShift", "pfMET_sysShift_phi",
    "pfMET_t1", "pfMET_t1p2", "pfMET_t01", "pfMET_t01p2", "pfNoPUMET", "pfMVAMET",
    "Njets", "Nbtags", "Nphotons", "Nmuons", "Nelectrons",
    "Ngamma", "Nfake",
    "HT", "HT_jets", "hadronic_pt", 
    "w_mT", "w_mT_t1", "w_mT_t1p2", "w_mT_t01", "w_mT_t01p2", "w_mT_nopumet", "w_mT_mvamet",
    "m3", "m3_uncorr",
    "ele_pt", "ele_phi", "ele_eta", "ele_mvaTrigV0", "ele_relIso",
    "muon_pt", "muon_phi", "muon_eta", "muon_relIso",
    "leadPhotonEt", "leadPhotonEta", "leadPhotonPhi", "leadChargedHadronIso", "leadSigmaIetaIeta", "lead_nPixelSeeds", "leadMVAregEnergy", "leadMVAregErr",
    "leadNeutralHadronIso", "leadPhotonIso",
    "trailPhotonEt", "trailPhotonEta", "trailPhotonPhi", "trailChargedHadronIso", "trailSigmaIetaIeta", "trail_nPixelSeeds", "trailMVAregEnergy", "trailMVAregErr",
    "trailNeutralHadronIso", "trailPhotonIso",
    "photon_invmass", "photon_dR", "photon_dPhi", "diEMpT", "diJetPt",
    "mLepGammaLead", "mLepGammaTrail", "mLepGammaGamma",
    "dR_leadPhoton_l", "dR_leadPhoton_b_min",
    "dEta_leadPhoton_l", "dEta_leadPhoton_b_min",
    "dPhi_leadPhoton_l", "dPhi_leadPhoton_b_min",
    "cosTheta_leadPhoton_l", "cosTheta_leadPhoton_b_min",
    "dR_trailPhoton_l", "dR_trailPhoton_b_min",
    "dEta_trailPhoton_l", "dEta_trailPhoton_b_min",
    "dPhi_trailPhoton_l", "dPhi_trailPhoton_b_min",
    "cosTheta_trailPhoton_l", "cosTheta_trailPhoton_b_min",
    "dPhi_met_l", "dPhi_met_ht",
    "dPhi_met_t1_l", "dPhi_met_t1_ht",
    "dPhi_met_t1p2_l", "dPhi_met_t1p2_ht",
    "dPhi_met_t01_l", "dPhi_met_t01_ht",
    "dPhi_met_t01p2_l", "dPhi_mett01p2_ht",
    "dPhi_nopumet_l", "dPhi_nopumet_ht",
    "dPhi_mvamet_l", "dPhi_mvamet_ht",
    "dPhi_genmet_l", "dPhi_genmet_ht",
    "jet1_pt", "jet2_pt", "jet3_pt", "jet4_pt",
    "btag1_pt", "btag2_pt",
    "max_csv", "submax_csv", "min_csv",
    "nPV",
    "metFilterBit",
    "runNumber", "eventNumber", "lumiBlock", "jentry"};
    
  map<TString, float> treeMap;
  for(int i = 0; i < nTreeVariables; i++) treeMap[varNames[i]] = 0.;

  vector<TTree*> signalTrees, fakeTrees,
    eQCDTrees, eQCDfakeTrees,
    muQCDTrees, muQCDfakeTrees;

  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_signalTree", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    signalTrees.push_back(tree);
  }
  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_fakeTree", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    fakeTrees.push_back(tree);
  }

  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_eQCDTree", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    eQCDTrees.push_back(tree);
  }
  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_eQCDfakeTree", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    eQCDfakeTrees.push_back(tree);
  }

  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_muQCDTree", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    muQCDTrees.push_back(tree);
  }
  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_muQCDfakeTree", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    muQCDfakeTrees.push_back(tree);
  }
      
  ScaleFactorInfo sf(btagger);

  bool quitAfterProcessing = false;

  Long64_t nEntries = fTree->GetEntries();
  cout << "Total events in files : " << nEntries << endl;
  cout << "Events to be processed : " << processNEvents << endl;

  vector<susy::Muon*> tightMuons, looseMuons;
  vector<susy::Electron*> tightEles, looseEles;
  vector<susy::PFJet*> pfJets, btags;
  vector<TLorentzVector> pfJets_corrP4, btags_corrP4;
  vector<float> csvValues;
  vector<susy::Photon*> photons;
  vector<BtagInfo> tagInfos;

  // start event looping
  Long64_t jentry = 0;
  while(jentry != processNEvents && event.getEntry(jentry++) != 0) {

    if(printLevel > 0 || (printInterval > 0 && (jentry >= printInterval && jentry%printInterval == 0))) {
      cout << int(jentry) << " events processed with run = " << event.runNumber << ", event = " << event.eventNumber << endl;
    }
    
    if(useSyncFile) {
      bool sync = false;
      for(unsigned int i = 0; i < syncRuns.size(); i++) {
	//if(event.runNumber == syncRuns[i] && event.luminosityBlockNumber == syncLumi[i] && event.eventNumber == syncEvents[i]) {
	if(event.runNumber == syncRuns[i] && event.eventNumber == syncEvents[i]) {
	  sync = true;
	  //Print(*event);
	  break;
	}
      }
      if(!sync) continue;

      //if(nCnt[0][0] == (syncRuns.size() - 1)) quitAfterProcessing = true;
    }

    if(singleEvent) {
      if(event.runNumber != single_run || event.luminosityBlockNumber != single_lumi || event.eventNumber != single_event) continue;
      //Print(event);
      quitAfterProcessing = true;
    }

    FillMetFilter2D(event, h_metFilter);

    nCnt[0][0]++; // events

    if(useJson && event.isRealData && !IsGoodLumi(event.runNumber, event.luminosityBlockNumber)) continue;
    nCnt[1][0]++;

    if(event.isRealData) {
      if(event.passMetFilters() != 1 ||
	 event.passMetFilter(susy::kEcalLaserCorr) != 1 ||
	 event.passMetFilter(susy::kManyStripClus53X) != 1 ||
	 event.passMetFilter(susy::kTooManyStripClus53X) != 1) {
	nCnt[21][0]++;
	continue;
      }
    }

    int nPVertex = GetNumberPV(event);
    if(nPVertex == 0) {
      nCnt[22][0]++;
      continue;
    }

    for(int qcdMode = kSignal; qcdMode < kNumSearchModes; qcdMode++) {

      for(int photonMode = kSignalPhotons; photonMode < kNumPhotonModes; photonMode++) {

	float HT = 0.;
	
	tightMuons.clear();
	looseMuons.clear();
	tightEles.clear();
	looseEles.clear();
	pfJets.clear();
	btags.clear();
	pfJets_corrP4.clear();
	btags_corrP4.clear();
	csvValues.clear();
	photons.clear();
	tagInfos.clear();
	
	findMuons(event, tightMuons, looseMuons, HT, qcdMode);
	if(tightMuons.size() > 1 || looseMuons.size() > 0) {
	  nCnt[23][qcdMode]++;
	  continue;
	}
	
	findElectrons(event, tightMuons, looseMuons, tightEles, looseEles, HT, qcdMode);
	if(tightEles.size() > 1 || looseEles.size() > 0) {
	  nCnt[29][qcdMode]++;
	  continue;
	}
	
	if(tightMuons.size() + tightEles.size() != 1) {
	  nCnt[24][qcdMode]++;
	  continue;
	}
	if(looseMuons.size() + looseEles.size() != 0) {
	  nCnt[26][qcdMode]++;
	  continue;
	}
	
	bool passHLT = true;
	if(useTrigger) {
	  if(tightEles.size() == 1) passHLT = PassTriggers(1);

	  else if(tightMuons.size() == 1) {
	    if(qcdMode == kSignal) passHLT = PassTriggers(2);
	    if(kSignal == kMuonQCD) passHLT = PassTriggers(3);
	  }
	}
	if(!passHLT) {
	  nCnt[25][qcdMode]++;
	  continue;
	}
	
	findPhotons(event, 
		    photons,
		    tightMuons, looseMuons,
		    tightEles, looseEles,
		    HT,
		    photonMode);

	float HT_jets = 0.;
	TLorentzVector hadronicSystem(0., 0., 0., 0.);
	
	findJets(event, 
		 tightMuons, looseMuons,
		 tightEles, looseEles,
		 photons,
		 pfJets, btags,
		 sf,
		 tagInfos, csvValues, 
		 pfJets_corrP4, btags_corrP4, 
		 HT_jets, hadronicSystem);
	
	SetTreeValues(treeMap,
		      tightMuons, tightEles, 
		      pfJets, btags,
		      photons,
		      pfJets_corrP4, btags_corrP4,
		      csvValues,
		      hadronicSystem,
		      HT, HT_jets,
		      nPVertex,
		      0, 0, 0, 0,
		      jentry);
	
	////////////////////
	
	for(int chan = 0; chan < nChannels; chan++) {
	  
	  if(pfJets.size() < nJetReq[chan]) continue;
	  if((nBtagInclusive[chan] && btags.size() < nBtagReq[chan]) || (!nBtagInclusive[chan] && btags.size() != nBtagReq[chan])) continue;
	  
	  if(tightEles.size() != nEleReq[chan]) continue;
	  if(tightMuons.size() != nMuonReq[chan]) continue;
	  
	  if(photonMode == kSignalPhotons) {
	    if(qcdMode == kSignal) {
	      nCnt[2][chan]++;
	      signalTrees[chan]->Fill();
	    }
	    else if(qcdMode == kElectronQCD) {
	      nCnt[3][chan]++;
	      eQCDTrees[chan]->Fill();
	    }
	    else if(qcdMode == kMuonQCD) {
	      nCnt[4][chan]++;
	      muQCDTrees[chan]->Fill();
	    }
	  }
	  
	  if(photonMode == kFakePhotons) {
	    if(qcdMode == kSignal) {
	      nCnt[5][chan]++;
	      fakeTrees[chan]->Fill();
	    }
	    else if(qcdMode == kElectronQCD) {
	      nCnt[6][chan]++;
	      eQCDfakeTrees[chan]->Fill();
	    }
	    else if(qcdMode == kMuonQCD) {
	      nCnt[7][chan]++;
	      muQCDfakeTrees[chan]->Fill();
	    }
	  }

	} // loop over jet/btag req channels
    
	///////////////////////////////////
    
      } // for photon modes

    } // for qcd modes

    if(quitAfterProcessing) break;
  } // for entries
  
  cout << "-------------------Job Summary-----------------" << endl;
  cout << "Total_events         : " << nCnt[0][0] << endl;
  cout << "in_JSON              : " << nCnt[1][0] << endl;
  cout << "-----------------------------------------------" << endl;
  cout << endl;
  for(int i = 0; i < nChannels; i++) {
    cout << "--------------- " << channels[i] << " Requirement ----------------" << endl;
    cout << "Signal               " << channels[i] << " events : " << nCnt[2][i] << endl;
    cout << "eQCD                 " << channels[i] << " events : " << nCnt[3][i] << endl;
    cout << "muQCD                " << channels[i] << " events : " << nCnt[4][i] << endl;
    cout << "fake      " << channels[i] << " events : " << nCnt[5][i] << endl;
    cout << "eQCDfake  " << channels[i] << " events : " << nCnt[6][i] << endl;
    cout << "muQCDfake " << channels[i] << " events : " << nCnt[7][i] << endl;
  }
  cout << "-----------------------------------------------" << endl;
  cout << endl;
  cout << "----------------Continues, info----------------" << endl;
  cout << "fail MET filters         : " << nCnt[21][0] << endl;
  cout << "No primary vertex        : " << nCnt[22][0] << endl;
  cout << "Fail signal HLT          : " << nCnt[25][0] << endl;
  cout << "Fail eQCD HLT            : " << nCnt[25][1] << endl;
  cout << "Fail muQCD HLT           : " << nCnt[25][2] << endl;
  cout << "-----------------------------------------------" << endl;
  cout << endl;

  out->cd();
  out->Write();
  out->Close();

}

void SusyEventAnalyzer::Acceptance() {

  const int NCNT = 50;
  int nCnt[NCNT][nChannels];
  for(int i = 0; i < NCNT; i++) {
    for(int j = 0; j < nChannels; j++) {
      nCnt[i][j] = 0;
    }
  }
  
  TString output_code_t = FormatName(scan);

  // open histogram file and define histograms
  TFile * out = new TFile("signal_contamination"+output_code_t+".root", "RECREATE");
  out->cd();

  TH1D * h_nEvents = new TH1D("nEvents"+output_code_t, "nEvents"+output_code_t, 1, 0, 1);

  TH2D * h_whizard_phaseSpace = new TH2D("whizard_phaseSpace"+output_code_t, "whizard_phaseSpace"+output_code_t, 500, 0, 1000, 500, 0, 5);
  TH2D * h_madgraph_phaseSpace = new TH2D("madgraph_phaseSpace"+output_code_t, "madgraph_phaseSpace"+output_code_t, 500, 0, 1000, 500, 0, 5);

  const int nTreeVariables = 132;

  TString varNames[nTreeVariables] = {
    "pfMET", "pfMET_x", "pfMET_y", "pfMET_phi",
    "pfMET_sysShift", "pfMET_sysShift_phi",
    "pfMET_t1", "pfMET_t1p2", "pfMET_t01", "pfMET_t01p2", "pfNoPUMET", "pfMVAMET", "genMET",
    "Njets", "Nbtags", "Nphotons", "Nmuons", "Nelectrons",
    "Ngamma", "Nfake",
    "HT", "HT_jets", "hadronic_pt", 
    "w_mT", "w_mT_t1", "w_mT_t1p2", "w_mT_t01", "w_mT_t01p2", "w_mT_nopumet", "w_mT_mvamet", "w_mT_genmet",
    "m3", "m3_uncorr",
    "ele_pt", "ele_phi", "ele_eta", "ele_mvaTrigV0", "ele_relIso",
    "muon_pt", "muon_phi", "muon_eta", "muon_relIso",
    "leadPhotonEt", "leadPhotonEta", "leadPhotonPhi", "leadChargedHadronIso", "leadSigmaIetaIeta", "lead_nPixelSeeds", "leadMVAregEnergy", "leadMVAregErr",
    "leadNeutralHadronIso", "leadPhotonIso",
    "trailPhotonEt", "trailPhotonEta", "trailPhotonPhi", "trailChargedHadronIso", "trailSigmaIetaIeta", "trail_nPixelSeeds", "trailMVAregEnergy", "trailMVAregErr",
    "trailNeutralHadronIso", "trailPhotonIso",
    "photon_invmass", "photon_dR", "photon_dPhi", "diEMpT", "diJetPt",
    "mLepGammaLead", "mLepGammaTrail", "mLepGammaGamma",
    "dR_leadPhoton_l", "dR_leadPhoton_b_min",
    "dEta_leadPhoton_l", "dEta_leadPhoton_b_min",
    "dPhi_leadPhoton_l", "dPhi_leadPhoton_b_min",
    "cosTheta_leadPhoton_l", "cosTheta_leadPhoton_b_min",
    "dR_trailPhoton_l", "dR_trailPhoton_b_min",
    "dEta_trailPhoton_l", "dEta_trailPhoton_b_min",
    "dPhi_trailPhoton_l", "dPhi_trailPhoton_b_min",
    "cosTheta_trailPhoton_l", "cosTheta_trailPhoton_b_min",
    "dPhi_met_l", "dPhi_met_ht",
    "dPhi_met_t1_l", "dPhi_met_t1_ht",
    "dPhi_met_t1p2_l", "dPhi_met_t1p2_ht",
    "dPhi_met_t01_l", "dPhi_met_t01_ht",
    "dPhi_met_t01p2_l", "dPhi_mett01p2_ht",
    "dPhi_nopumet_l", "dPhi_nopumet_ht",
    "dPhi_mvamet_l", "dPhi_mvamet_ht",
    "dPhi_genmet_l", "dPhi_genmet_ht",
    "jet1_pt", "jet2_pt", "jet3_pt", "jet4_pt",
    "btag1_pt", "btag2_pt",
    "max_csv", "submax_csv", "min_csv",
    "nPV",
    "pileupWeight", "pileupWeightErr", "pileupWeightUp", "pileupWeightDown",
    "btagWeight", "btagWeightUp", "btagWeightDown", "btagWeightErr",
    "metFilterBit",
    "ttbarDecayMode",
    "overlaps_whizard", "overlaps_madgraph",
    "TopPtReweighting", "TopPtReweighting_ttHbb",
    "leadMatchGamma", "leadMatchElectron", "leadMatchJet",
    "trailMatchGamma", "trailMatchElectron", "trailMatchJet"};
    
  map<TString, float> treeMap;
  for(int i = 0; i < nTreeVariables; i++) treeMap[varNames[i]] = 0.;

  vector<TTree*> signalTrees, signalTrees_JECup, signalTrees_JECdown;
  vector<TTree*> fakeTrees, fakeTrees_JECup, fakeTrees_JECdown;
  vector<TTree*> eQCDTrees, eQCDfakeTrees,
    muQCDTrees, muQCDfakeTrees;
  
  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_signalTree", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    signalTrees.push_back(tree);
  }
  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_signalTree_JECup", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    signalTrees_JECup.push_back(tree);
  }
  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_signalTree_JECdown", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    signalTrees_JECdown.push_back(tree);
  }

  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_fakeTree", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    fakeTrees.push_back(tree);
  }
  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_fakeTree_JECup", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    fakeTrees_JECup.push_back(tree);
  }
  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_fakeTree_JECdown", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    fakeTrees_JECdown.push_back(tree);
  }

  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_eQCDTree", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    eQCDTrees.push_back(tree);
  }
  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_eQCDfakeTree", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    eQCDfakeTrees.push_back(tree);
  }

  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_muQCDTree", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    muQCDTrees.push_back(tree);
  }
  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_muQCDfakeTree", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    muQCDfakeTrees.push_back(tree);
  }

  ScaleFactorInfo sf(btagger);
  TFile * btagEfficiency = new TFile("btagEfficiency"+output_code_t+".root", "READ");
  sf.SetTaggingEfficiencies((TH1F*)btagEfficiency->Get("lEff"+output_code_t), (TH1F*)btagEfficiency->Get("cEff"+output_code_t), (TH1F*)btagEfficiency->Get("bEff"+output_code_t));

  // get pileup weights
  TFile * puFile = new TFile("pileupReweighting"+output_code_t+".root", "READ");
  TH1F * puWeights = (TH1F*)puFile->Get("puWeights"+output_code_t);
  TH1F * puWeights_up = (TH1F*)puFile->Get("puWeights_up"+output_code_t);
  TH1F * puWeights_down = (TH1F*)puFile->Get("puWeights_down"+output_code_t);

  Long64_t nEntries = fTree->GetEntries();
  cout << "Total events in files : " << nEntries << endl;
  cout << "Events to be processed : " << processNEvents << endl;
  h_nEvents->Fill(0., (Double_t)nEntries);

  vector<susy::Muon*> tightMuons, looseMuons;
  vector<susy::Electron*> tightEles, looseEles;
  vector<susy::PFJet*> pfJets, btags;
  vector<TLorentzVector> pfJets_corrP4, btags_corrP4;
  vector<float> csvValues;
  vector<susy::Photon*> photons;
  vector<BtagInfo> tagInfos;

  // start event looping
  Long64_t jentry = 0;
  while(jentry != processNEvents && event.getEntry(jentry++) != 0) {

    if(printLevel > 0 || (printInterval > 0 && (jentry >= printInterval && jentry%printInterval == 0))) {
      cout << int(jentry) << " events processed with run = " << event.runNumber << ", event = " << event.eventNumber << endl;
    }

    nCnt[0][0]++; // events

    float numTrueInt = -1.;
    susy::PUSummaryInfoCollection::const_iterator iBX = event.pu.begin();
    bool foundInTimeBX = false;
    while((iBX != event.pu.end()) && !foundInTimeBX) {
      if(iBX->BX == 0) {
	numTrueInt = iBX->trueNumInteractions;
	foundInTimeBX = true;
      }
      ++iBX;
    }

    float eventWeight = 0.;
    float eventWeightErr = 0.;
    float eventWeightUp = 0.;
    float eventWeightDown = 0.;
    if(numTrueInt >= 0.) {
      int binNum = puWeights->GetXaxis()->FindBin(numTrueInt);
      eventWeight = puWeights->GetBinContent(binNum);
      eventWeightErr = puWeights->GetBinError(binNum);
      eventWeightUp = puWeights_up->GetBinContent(binNum);
      eventWeightDown = puWeights_down->GetBinContent(binNum);
    }

    if(!doPileupReweighting) {
      eventWeight = 1.;
      eventWeightErr = 0.;
      eventWeightUp = 1.;
      eventWeightDown = 1.;
    }

    int nPVertex = GetNumberPV(event);
    if(nPVertex == 0) continue;
    
    fill_whizard_phaseSpace(h_whizard_phaseSpace);
    fill_madgraph_phaseSpace(h_madgraph_phaseSpace);

    for(int qcdMode = kSignal; qcdMode < kNumSearchModes; qcdMode++) {
      for(int jetSyst = kCentral; jetSyst < kNumJetSytematics; jetSyst++) {
	for(int photonMode = kSignalPhotons; photonMode < kNumPhotonModes; photonMode++) {

	  if(qcdMode != kSignal && jetSyst != kCentral) continue;

	  float HT = 0.;
	  
	  tightMuons.clear();
	  looseMuons.clear();
	  tightEles.clear();
	  looseEles.clear();
	  pfJets.clear();
	  btags.clear();
	  pfJets_corrP4.clear();
	  btags_corrP4.clear();
	  csvValues.clear();
	  photons.clear();
	  tagInfos.clear();
	  
	  findMuons(event, tightMuons, looseMuons, HT, qcdMode);
	  if(tightMuons.size() > 1 || looseMuons.size() > 0) continue;
	  
	  findElectrons(event, tightMuons, looseMuons, tightEles, looseEles, HT, qcdMode);
	  if(tightEles.size() > 1 || looseEles.size() > 0) continue;
	  
	  if(tightMuons.size() + tightEles.size() != 1) continue;
	  if(looseMuons.size() + looseEles.size() != 0) continue;
	  
	  bool passHLT = true;
	  if(useTrigger) {
	    if(tightEles.size() == 1) passHLT = PassTriggers(1);

	    else if(tightMuons.size() == 1) {
	      if(qcdMode == kSignal) passHLT = PassTriggers(2);
	      if(kSignal == kMuonQCD) passHLT = PassTriggers(3);
	    }
	  }
	  if(!passHLT) continue;
	  
	  findPhotons(event, 
		      photons,
		      tightMuons, looseMuons,
		      tightEles, looseEles,
		      HT,
		      photonMode);

	  float HT_jets = 0.;
	  TLorentzVector hadronicSystem(0., 0., 0., 0.);
	  
	  findJets_inMC(event, 
			tightMuons, looseMuons,
			tightEles, looseEles,
			photons,
			pfJets, btags,
			sf,
			tagInfos, csvValues, 
			pfJets_corrP4, btags_corrP4, 
			HT_jets, hadronicSystem,
			jetSyst);
	  
	  float btagWeight[nChannels];
	  float btagWeightUp[nChannels];
	  float btagWeightDown[nChannels];
	  float btagWeightError[nChannels];
	  for(int chan = 0; chan < nChannels; chan++) {
	    BtagWeight * tagWeight = new BtagWeight(nBtagReq[chan]);
	    pair<float, float> weightResult = tagWeight->weight(tagInfos, btags.size(), 0., false, nBtagInclusive[chan]);
	    btagWeight[chan] = weightResult.first;
	    btagWeightError[chan] = weightResult.second;
	    
	    btagWeightUp[chan] = (tagWeight->weight(tagInfos, btags.size(), 1., true, nBtagInclusive[chan])).first;
	    btagWeightDown[chan] = (tagWeight->weight(tagInfos, btags.size(), -1., true, nBtagInclusive[chan])).first;
	    
	    delete tagWeight;
	  }
	  
	  SetTreeValues(treeMap,
			tightMuons, tightEles, 
			pfJets, btags,
			photons,
			pfJets_corrP4, btags_corrP4,
			csvValues,
			hadronicSystem,
			HT, HT_jets,
			nPVertex,
			eventWeight, eventWeightErr, eventWeightUp, eventWeightDown,
			0);
	  
	  ////////////////////
	  
	  for(int chan = 0; chan < nChannels; chan++) {
	    
	    if(pfJets.size() < nJetReq[chan]) continue;
	    if((nBtagInclusive[chan] && btags.size() < nBtagReq[chan]) || (!nBtagInclusive[chan] && btags.size() != nBtagReq[chan])) continue;
	    
	    if(tightEles.size() != nEleReq[chan]) continue;
	    if(tightMuons.size() != nMuonReq[chan]) continue;
	    
	    treeMap["btagWeight"] = btagWeight[chan];
	    treeMap["btagWeightErr"] = btagWeightError[chan];
	    treeMap["btagWeightUp"] = btagWeightUp[chan];
	    treeMap["btagWeightDown"] = btagWeightDown[chan];

	    if(photonMode == kSignalPhotons) {
	      if(qcdMode == kSignal) {
		
		if(jetSyst == kCentral) {
		  nCnt[2][chan]++;
		  signalTrees[chan]->Fill();
		}
		else if(jetSyst == kJECup) signalTrees_JECup[chan]->Fill();
		else if(jetSyst == kJECdown) signalTrees_JECdown[chan]->Fill();

	      }
	      else if(qcdMode == kElectronQCD) {
		nCnt[3][chan]++;
		eQCDTrees[chan]->Fill();
	      }
	      else if(qcdMode == kMuonQCD) {
		nCnt[4][chan]++;
		muQCDTrees[chan]->Fill();
	      }
	    }
	    
	    if(photonMode == kFakePhotons) {
	      if(qcdMode == kSignal) {
		if(jetSyst == kCentral) {
		  nCnt[5][chan]++;
		  fakeTrees[chan]->Fill();
		}
		else if(jetSyst == kJECup) fakeTrees_JECup[chan]->Fill();
		else if(jetSyst == kJECdown) fakeTrees_JECdown[chan]->Fill();
	      }
	      else if(qcdMode == kElectronQCD && jetSyst == kCentral) {
		nCnt[6][chan]++;
		eQCDfakeTrees[chan]->Fill();
	      }
	      else if(qcdMode == kMuonQCD && jetSyst == kCentral) {
		nCnt[7][chan]++;
		muQCDfakeTrees[chan]->Fill();
	      }
	    }

	  } // for channels

	} // for photon modes
	  
      } // for jet systematic modes
	
    } // for qcd modes

  } // for entries

  cout << "-------------------Job Summary-----------------" << endl;
  cout << "Total_events         : " << nCnt[0][0] << endl;
  cout << "-----------------------------------------------" << endl;
  cout << endl;
  for(int i = 0; i < nChannels; i++) {
    cout << "---------------- " << channels[i] << " Requirement ----------------" << endl;
    cout << "Signal               " << channels[i] << " events : " << nCnt[2][i] << endl;
    cout << "eQCD                 " << channels[i] << " events : " << nCnt[3][i] << endl;
    cout << "muQCD                " << channels[i] << " events : " << nCnt[4][i] << endl;
    cout << "fake                 " << channels[i] << " events : " << nCnt[5][i] << endl;
    cout << "eQCDfake             " << channels[i] << " events : " << nCnt[6][i] << endl;
    cout << "muQCDfake            " << channels[i] << " events : " << nCnt[7][i] << endl;
  }
  cout << endl;
  cout << "----------------Continues, info----------------" << endl;
 
  puFile->Close();
  btagEfficiency->Close();

  out->Write();
  out->Close();

}

void SusyEventAnalyzer::GeneratorInfo() {

  TString output_code_t = FormatName(scan);

  // open histogram file and define histograms
  TFile * out = new TFile("generator_info"+output_code_t+".root", "RECREATE");
  out->cd();

  TH1D * h_stop_pt = new TH1D("stop_pt", "stop_pt", 400, 0, 2000);
  TH1D * h_top_pt = new TH1D("top_pt", "top_pt", 400, 0, 2000);
  TH1D * h_bottom_pt = new TH1D("bottom_pt", "bottom_pt", 400, 0, 2000);
  TH1D * h_w_pt = new TH1D("w_pt", "w_pt", 400, 0, 2000);
  TH1D * h_bino_pt = new TH1D("bino_pt", "bino_pt", 400, 0, 2000);
  TH1D * h_photon_pt = new TH1D("photon_pt", "photon_pt", 400, 0, 2000);
  TH1D * h_muon_pt = new TH1D("muon_pt", "muon_pt", 400, 0, 2000);
  TH1D * h_ele_pt = new TH1D("ele_pt", "ele_pt", 400, 0, 2000);
  TH1D * h_genMET = new TH1D("genMET", "genMET", 400, 0, 2000);
  TH1D * h_top_invmass = new TH1D("top_invmass", "top_invmass", 400, 0, 2000);

  // compare to older samples
  TH1D * h_squark_olderCompare_pt = new TH1D("squark_olderCompare_pt", "squark_olderCompare_pt", 400, 0, 2000);
  TH1D * h_gluino_olderCompare_pt = new TH1D("gluino_olderCompare_pt", "gluino_olderCompare_pt", 400, 0, 2000);
  TH1D * h_stop_olderCompare_pt = new TH1D("stop_olderCompare_pt", "stop_olderCompare_pt", 400, 0, 2000);
  TH1D * h_bino_olderCompare_pt = new TH1D("bino_olderCompare_pt", "bino_olderCompare_pt", 400, 0, 2000);
    
  TH1D * h_charm_pt = new TH1D("charm_pt", "charm_pt", 400, 0, 2000);
  TH1D * h_strange_pt = new TH1D("strange_pt", "strange_pt", 400, 0, 2000);
  TH1D * h_up_pt = new TH1D("up_pt", "up_pt", 400, 0, 2000);
  TH1D * h_down_pt = new TH1D("down_pt", "down_pt", 400, 0, 2000);

  TTree * tree_dalitz = new TTree("dalitzTree", "dalitzTree");
  Float_t mbw, mbBino;
  tree_dalitz->Branch("mbW", &mbw, "mbw/F");
  tree_dalitz->Branch("mbBino", &mbBino, "mbBino/F");

  TH1D * h_nPhotons = new TH1D("nPhotons", "nPhotons", 4, 0, 4);

  Long64_t nEntries = fTree->GetEntries();
  cout << "Total events in files : " << nEntries << endl;
  cout << "Events to be processed : " << processNEvents << endl;

  // start event looping
  Long64_t jentry = 0;
  while(jentry != processNEvents && event.getEntry(jentry++) != 0) {

    if(printLevel > 0 || (printInterval > 0 && (jentry >= printInterval && jentry%printInterval == 0))) {
      cout << int(jentry) << " events processed with run = " << event.runNumber << ", event = " << event.eventNumber << endl;
    }

    // compare to older samples
    for(vector<susy::Particle>::iterator it = event.genParticles.begin(); it != event.genParticles.end(); it++) {
      if(abs(it->pdgId) >= 1000001 && abs(it->pdgId) <= 1000003) h_squark_olderCompare_pt->Fill(it->momentum.Pt());
      if(abs(it->pdgId) == 1000021) h_gluino_olderCompare_pt->Fill(it->momentum.Pt());
      if(abs(it->pdgId) == 1000006) h_stop_olderCompare_pt->Fill(it->momentum.Pt());
      if(abs(it->pdgId) == 1000022) h_bino_olderCompare_pt->Fill(it->momentum.Pt());
    }
      
    susy::Particle * stop = 0;
    susy::Particle * antistop = 0;

    susy::Particle * top = 0;
    susy::Particle * antitop = 0;

    for(vector<susy::Particle>::iterator it = event.genParticles.begin(); it != event.genParticles.end(); it++) {
      if(abs(it->pdgId) == 1000006 && it->status == 3 ) {
	h_stop_pt->Fill(it->momentum.Pt());
	if(it->pdgId == 1000006 && !stop) stop = &*it;
	else if(it->pdgId == -1000006 && !antistop) antistop = &*it;
      }
      if(stop && antistop) break;
    }
    if(!stop || !antistop) continue;

    for(vector<susy::Particle>::iterator it = event.genParticles.begin(); it != event.genParticles.end(); it++) {
      if(abs(it->pdgId) == 6 && it->status == 3) {
	h_top_pt->Fill(it->momentum.Pt());
	h_top_invmass->Fill(it->momentum.M());
	if(it->pdgId == 6 && !top) top = &*it;
	else if(it->pdgId == -6 && !antitop) antitop = &*it;
      }
      if(top && antitop) break;
    }

    if(!top) {
      TLorentzVector bW_pair(0, 0, 0, 0);
      TLorentzVector bBino_pair(0, 0, 0, 0);

      int n_bW = 0;
      int n_bBino = 0;

      for(vector<susy::Particle>::iterator it = event.genParticles.begin(); it != event.genParticles.end(); it++) {
	if(it->mother == stop && it->status == 3 && (abs(it->pdgId) == 5 || abs(it->pdgId) == 24)) {
	  bW_pair += it->momentum;
	  n_bW++;
	}
	if(it->mother == stop && it->status == 3 && (abs(it->pdgId) == 5 || abs(it->pdgId) == 1000022)) {
	  bBino_pair += it->momentum;
	  n_bBino++;
	}
      }

      if(n_bW == 2 && n_bBino == 2) {
	mbw = bW_pair.M();
	mbBino = bBino_pair.M();
	tree_dalitz->Fill();
      }
    }

    if(!antitop) {
      TLorentzVector bW_pair(0, 0, 0, 0);
      TLorentzVector bBino_pair(0, 0, 0, 0);

      int n_bW = 0;
      int n_bBino = 0;

      for(vector<susy::Particle>::iterator it = event.genParticles.begin(); it != event.genParticles.end(); it++) {
	if(it->mother == antistop && it->status == 3 && (abs(it->pdgId) == 5 || abs(it->pdgId) == 24)) {
	  bW_pair += it->momentum;
	  n_bW++;
	}
	if(it->mother == antistop && it->status == 3 && (abs(it->pdgId) == 5 || abs(it->pdgId) == 1000022)) {
	  bBino_pair += it->momentum;
	  n_bBino++;
	}
      }

      if(n_bW == 2 && n_bBino == 2) {
	mbw = bW_pair.M();
	mbBino = bBino_pair.M();
	tree_dalitz->Fill();
      }

    }

    int nPhotons = 0;

    for(vector<susy::Particle>::iterator it = event.genParticles.begin(); it != event.genParticles.end(); it++) {

      if(abs(it->pdgId) == 4 && it->status == 3 && (it->mother == stop || it->mother == antistop)) h_charm_pt->Fill(it->momentum.Pt());
      if(abs(it->pdgId) == 3 && it->status == 3 && (it->mother == stop || it->mother == antistop)) h_strange_pt->Fill(it->momentum.Pt());
      if(abs(it->pdgId) == 2 && it->status == 3 && (it->mother == stop || it->mother == antistop)) h_up_pt->Fill(it->momentum.Pt());
      if(abs(it->pdgId) == 1 && it->status == 3 && (it->mother == stop || it->mother == antistop)) h_down_pt->Fill(it->momentum.Pt());
      
      if(abs(it->pdgId) == 5 && it->status == 3 && (it->mother == top || it->mother == stop || it->mother == antitop || it->mother == antistop)) h_bottom_pt->Fill(it->momentum.Pt());
      if(abs(it->pdgId) == 24 && it->status == 3 && (it->mother == top || it->mother == stop || it->mother == antitop || it->mother == antistop)) h_w_pt->Fill(it->momentum.Pt());
      if(abs(it->pdgId) == 1000022 && it->status == 3 && (it->mother == top || it->mother == stop || it->mother == antitop || it->mother == antistop)) h_bino_pt->Fill(it->momentum.Pt());
      if(abs(it->pdgId) == 22 && abs(it->mother->pdgId) == 1000022) {
	nPhotons++;
	h_photon_pt->Fill(it->momentum.Pt());
      }
      if(abs(it->pdgId) == 11 && it->status == 3 && abs(it->mother->pdgId) == 24) h_ele_pt->Fill(it->momentum.Pt());
      if(abs(it->pdgId) == 13 && it->status == 3 && abs(it->mother->pdgId) == 24) h_muon_pt->Fill(it->momentum.Pt());

    }

    susy::MET* genMet = &(event.metMap.find("genMetTrue")->second);

    h_genMET->Fill(genMet->met());
    h_nPhotons->Fill(nPhotons);

  } // for entries

  out->Write();
  out->Close();

}

void SusyEventAnalyzer::LeptonInfo() {

  TString output_code_t = FormatName(scan);

  // open histogram file and define histograms
  TFile * out = new TFile("lepton_info"+output_code_t+".root", "RECREATE");
  out->cd();

  Float_t pfmet, genmet;

  Float_t muon_gen_pt, muon_gen_eta;
  Float_t muon_reco_pt, muon_reco_eta, muon_reco_relIso;
  bool muon_reco_passTight, muon_reco_passLoose;
  
  TTree * muonTree = new TTree("muonTree", "muon tree");
  muonTree->Branch("pfmet", &pfmet, "pfmet/F");
  muonTree->Branch("genmet", &genmet, "genmet/F");
  muonTree->Branch("muon_gen_pt", &muon_gen_pt, "muon_gen_pt/F");
  muonTree->Branch("muon_gen_eta", &muon_gen_eta, "muon_gen_eta/F");
  muonTree->Branch("muon_reco_pt", &muon_reco_pt, "muon_reco_pt/F");
  muonTree->Branch("muon_reco_eta", &muon_reco_eta, "muon_reco_eta/F");
  muonTree->Branch("muon_reco_relIso", &muon_reco_relIso, "muon_reco_relIso/F");
  muonTree->Branch("muon_reco_passTight", &muon_reco_passTight, "muon_reco_passTight/O");
  muonTree->Branch("muon_reco_passLoose", &muon_reco_passLoose, "muon_reco_passLoose/O");
  
  Float_t ele_gen_pt, ele_gen_eta;
  Float_t ele_reco_pt, ele_reco_eta, ele_reco_relIso;
  bool ele_reco_passTight, ele_reco_passLoose;
  
  TTree * eleTree = new TTree("eleTree", "ele tree");
  eleTree->Branch("pfmet", &pfmet, "pfmet/F");
  eleTree->Branch("genmet", &genmet, "genmet/F");
  eleTree->Branch("ele_gen_pt", &ele_gen_pt, "ele_gen_pt/F");
  eleTree->Branch("ele_gen_eta", &ele_gen_eta, "ele_gen_eta/F");
  eleTree->Branch("ele_reco_pt", &ele_reco_pt, "ele_reco_pt/F");
  eleTree->Branch("ele_reco_eta", &ele_reco_eta, "ele_reco_eta/F");
  eleTree->Branch("ele_reco_relIso", &ele_reco_relIso, "ele_reco_relIso/F");
  eleTree->Branch("ele_reco_passTight", &ele_reco_passTight, "ele_reco_passTight/O");
  eleTree->Branch("ele_reco_passLoose", &ele_reco_passLoose, "ele_reco_passLoose/O");

  Long64_t nEntries = fTree->GetEntries();
  cout << "Total events in files : " << nEntries << endl;
  cout << "Events to be processed : " << processNEvents << endl;

  // start event looping
  Long64_t jentry = 0;
  while(jentry != processNEvents && event.getEntry(jentry++) != 0) {

    if(printLevel > 0 || (printInterval > 0 && (jentry >= printInterval && jentry%printInterval == 0))) {
      cout << int(jentry) << " events processed with run = " << event.runNumber << ", event = " << event.eventNumber << endl;
    }

    susy::MET * obj_pfmet  = &(event.metMap.find("pfMet")->second);
    susy::MET * obj_genmet = &(event.metMap.find("genMetTrue")->second);

    pfmet = obj_pfmet->met();
    genmet = obj_genmet->met();

    for(vector<susy::Particle>::iterator it = event.genParticles.begin(); it != event.genParticles.end(); it++) {

      if(abs(it->pdgId) == 11 && it->status == 1 && abs(it->mother->pdgId) == 24) {
	ele_gen_pt = it->momentum.Pt();
	ele_gen_eta = it->momentum.Eta();

	ele_reco_pt = -1000;

	map<TString, vector<susy::Electron> >::iterator eleMap = event.electrons.find("gsfElectrons");
	if(eleMap != event.electrons.end()) {
	  for(vector<susy::Electron>::iterator ele_it = eleMap->second.begin(); ele_it != eleMap->second.end(); ele_it++) {

	    if(deltaR(ele_it->momentum, it->momentum) > 0.01) continue;

	    if((int)ele_it->gsfTrackIndex >= (int)(event.tracks).size() || (int)ele_it->gsfTrackIndex < 0) continue;

	    ele_reco_passTight = isTightElectron(*ele_it, 
						 event.superClusters, 
						 event.rho25, 
						 d0correction(event.vertices[0].position, event.tracks[ele_it->gsfTrackIndex]), 
						 dZcorrection(event.vertices[0].position, event.tracks[ele_it->gsfTrackIndex]));
	    
	    ele_reco_passLoose = isLooseElectron(*ele_it,
						 event.superClusters, 
						 event.rho25, 
						 d0correction(event.vertices[0].position, event.tracks[ele_it->gsfTrackIndex])); 

	    ele_reco_pt = ele_it->momentum.Pt();
	    ele_reco_eta = event.superClusters[ele_it->superClusterIndex].position.Eta();

	    float ea;
	    if(ele_reco_eta < 1.0) ea = 0.13;
	    else if(ele_reco_eta < 1.479) ea = 0.14;
	    else if(ele_reco_eta < 2.0) ea = 0.07;
	    else if(ele_reco_eta < 2.2) ea = 0.09;
	    else if(ele_reco_eta < 2.3) ea = 0.11;
	    else if(ele_reco_eta < 2.4) ea = 0.11;
	    else ea = 0.14;

	    ele_reco_relIso = ele_it->photonIso + ele_it->neutralHadronIso - event.rho25*ea;
	    if(ele_reco_relIso < 0) ele_reco_relIso = 0;
	    ele_reco_relIso += ele_it->chargedHadronIso;

	  }

	}

	if(ele_reco_pt < 0.) continue; // no match

	eleTree->Fill();
      }

      if(abs(it->pdgId) == 13 && it->status == 1 && abs(it->mother->pdgId) == 24) {
	muon_gen_pt = it->momentum.Pt();
	muon_gen_eta = it->momentum.Eta();

	muon_reco_pt = -1000;

	map<TString, vector<susy::Muon> >::iterator muMap = event.muons.find("muons");
	if(muMap != event.muons.end()) {
	  for(vector<susy::Muon>::iterator mu_it = muMap->second.begin(); mu_it != muMap->second.end(); mu_it++) {
	    
	    if(deltaR(mu_it->momentum, it->momentum) > 0.01) continue;

	    if((int)mu_it->bestTrackIndex() >= (int)(event.tracks).size() || (int)mu_it->bestTrackIndex() < 0) continue;

	    muon_reco_passTight = isTightMuon(*mu_it, 
					      event.tracks, 
					      d0correction(event.vertices[0].position, event.tracks[mu_it->bestTrackIndex()]), 
					      dZcorrection(event.vertices[0].position, event.tracks[mu_it->bestTrackIndex()]));
	    
	    muon_reco_passLoose = isVetoMuon(*mu_it);
	    
	    muon_reco_pt = mu_it->momentum.Pt();
	    muon_reco_eta = mu_it->momentum.Eta();

	    muon_reco_relIso = mu_it->sumNeutralHadronEt04 + mu_it->sumPhotonEt04 - 0.5*(mu_it->sumPUPt04);
	    if(muon_reco_relIso < 0) muon_reco_relIso = 0;
	    muon_reco_relIso += mu_it->sumChargedHadronPt04;

	  }
	}

	if(muon_reco_pt < 0.) continue; // no match

	muonTree->Fill();

      }

    } // for gen particles

  } // for entries

  out->Write();
  out->Close();

}

void SusyEventAnalyzer::ZGammaData() {

  TFile* out = new TFile("zgamma_"+outputName+"_"+btagger+".root", "RECREATE");
  out->cd();

  const int NCNT = 50;
  int nCnt[NCNT][nChannels];
  for(int i = 0; i < NCNT; i++) {
    for(int j = 0; j < nChannels; j++) {
      nCnt[i][j] = 0;
    }
  }

  ///////////////////////////////////////////////////
  // Define histograms to be filled for all events
  ///////////////////////////////////////////////////

  TString metFilterNames[susy::nMetFilters] = {
    "CSCBeamHalo",
    "HcalNoise",
    "EcalDeadCellTP",
    "EcalDeadCellBE",
    "TrackingFailure",
    "EEBadSC",
    "HcalLaserOccupancy",
    "HcalLaserEventList",
    "HcalLaserRECOUserStep",
    "EcalLaserCorr",
    "ManyStripClus53X",
    "TooManyStripClus53X",
    "LogErrorTooManyClusters",
    "LogErrorTooManyTripletsPairs",
    "LogErrorTooManySeeds",
    "EERingOfFire",
    "InconsistentMuon",
    "GreedyMuon"};

  TH2F* h_metFilter = new TH2F("metFilter", "MET Filter Failures", susy::nMetFilters, 0, susy::nMetFilters, susy::nMetFilters, 0, susy::nMetFilters);
  for(int i = 0; i < susy::nMetFilters; i++) {
    h_metFilter->GetXaxis()->SetBinLabel(i+1, metFilterNames[i]);
    h_metFilter->GetYaxis()->SetBinLabel(i+1, metFilterNames[i]);
  }

  /////////////////////////////////
  // Reweighting trees
  /////////////////////////////////

  const int nTreeVariables = 86;

  TString varNames[nTreeVariables] = {
    "pfMET", "pfMET_x", "pfMET_y", "pfMET_phi",
    "pfMET_sysShift", "pfMET_sysShift_phi",
    "pfMET_t1", "pfMET_t1p2", "pfMET_t01", "pfMET_t01p2", "pfNoPUMET", "pfMVAMET",
    "Njets", "Nbtags", "Nphotons",
    "Ngamma", "Nfake",
    "HT", "HT_jets", "hadronic_pt", 
    "ele1_pt", "ele1_phi", "ele1_eta", "ele1_mvaTrigV0", "ele1_relIso",
    "ele2_pt", "ele2_phi", "ele2_eta", "ele2_mvaTrigV0", "ele2_relIso",
    "muon1_pt", "muon1_phi", "muon1_eta", "muon1_relIso",
    "muon2_pt", "muon2_phi", "muon2_eta", "muon2_relIso",
    "z_mass", "z_pt", "z_eta", "z_phi",
    "leadPhotonEt", "leadPhotonEta", "leadPhotonPhi", "leadChargedHadronIso", "leadSigmaIetaIeta", "lead_nPixelSeeds", "leadMVAregEnergy", "leadMVAregErr",
    "leadNeutralHadronIso", "leadPhotonIso",
    "trailPhotonEt", "trailPhotonEta", "trailPhotonPhi", "trailChargedHadronIso", "trailSigmaIetaIeta", "trail_nPixelSeeds", "trailMVAregEnergy", "trailMVAregErr",
    "trailNeutralHadronIso", "trailPhotonIso",
    "photon_invmass", "photon_dR", "photon_dPhi", "diEMpT", "diJetPt",
    "mLepGammaLead", "mLepGammaTrail", "mLepGammaGamma",
    "jet1_pt", "jet2_pt", "jet3_pt", "jet4_pt",
    "btag1_pt", "btag2_pt",
    "max_csv", "submax_csv", "min_csv",
    "nPV",
    "metFilterBit",
    "runNumber", "eventNumber", "lumiBlock", "jentry"};
    
  map<TString, float> treeMap;
  for(int i = 0; i < nTreeVariables; i++) treeMap[varNames[i]] = 0.;

  vector<TTree*> signalTrees, fakeTrees,
    eQCDTrees, eQCDfakeTrees,
    muQCDTrees, muQCDfakeTrees;

  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_signalTree", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    signalTrees.push_back(tree);
  }
  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_fakeTree", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    fakeTrees.push_back(tree);
  }

  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_eQCDTree", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    eQCDTrees.push_back(tree);
  }
  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_eQCDfakeTree", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    eQCDfakeTrees.push_back(tree);
  }

  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_muQCDTree", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    muQCDTrees.push_back(tree);
  }
  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_muQCDfakeTree", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    muQCDfakeTrees.push_back(tree);
  }
      
  ScaleFactorInfo sf(btagger);

  bool quitAfterProcessing = false;

  Long64_t nEntries = fTree->GetEntries();
  cout << "Total events in files : " << nEntries << endl;
  cout << "Events to be processed : " << processNEvents << endl;

  vector<susy::Muon*> tightMuons, looseMuons;
  vector<susy::Electron*> tightEles, looseEles;
  vector<susy::PFJet*> pfJets, btags;
  vector<TLorentzVector> pfJets_corrP4, btags_corrP4;
  vector<float> csvValues;
  vector<susy::Photon*> photons;
  vector<BtagInfo> tagInfos;

  // start event looping
  Long64_t jentry = 0;
  while(jentry != processNEvents && event.getEntry(jentry++) != 0) {

    if(printLevel > 0 || (printInterval > 0 && (jentry >= printInterval && jentry%printInterval == 0))) {
      cout << int(jentry) << " events processed with run = " << event.runNumber << ", event = " << event.eventNumber << endl;
    }
    
    if(useSyncFile) {
      bool sync = false;
      for(unsigned int i = 0; i < syncRuns.size(); i++) {
	//if(event.runNumber == syncRuns[i] && event.luminosityBlockNumber == syncLumi[i] && event.eventNumber == syncEvents[i]) {
	if(event.runNumber == syncRuns[i] && event.eventNumber == syncEvents[i]) {
	  sync = true;
	  //Print(*event);
	  break;
	}
      }
      if(!sync) continue;

      //if(nCnt[0][0] == (syncRuns.size() - 1)) quitAfterProcessing = true;
    }

    if(singleEvent) {
      if(event.runNumber != single_run || event.luminosityBlockNumber != single_lumi || event.eventNumber != single_event) continue;
      //Print(event);
      quitAfterProcessing = true;
    }

    FillMetFilter2D(event, h_metFilter);

    nCnt[0][0]++; // events

    if(useJson && event.isRealData && !IsGoodLumi(event.runNumber, event.luminosityBlockNumber)) continue;
    nCnt[1][0]++;

    if(event.isRealData) {
      if(event.passMetFilters() != 1 ||
	 event.passMetFilter(susy::kEcalLaserCorr) != 1 ||
	 event.passMetFilter(susy::kManyStripClus53X) != 1 ||
	 event.passMetFilter(susy::kTooManyStripClus53X) != 1) {
	nCnt[21][0]++;
	continue;
      }
    }

    int nPVertex = GetNumberPV(event);
    if(nPVertex == 0) {
      nCnt[22][0]++;
      continue;
    }

    for(int qcdMode = kSignal; qcdMode < kNumSearchModes; qcdMode++) {

      for(int photonMode = kSignalPhotons; photonMode < kNumPhotonModes; photonMode++) {

	float HT = 0.;
	
	tightMuons.clear();
	looseMuons.clear();
	tightEles.clear();
	looseEles.clear();
	pfJets.clear();
	btags.clear();
	pfJets_corrP4.clear();
	btags_corrP4.clear();
	csvValues.clear();
	photons.clear();
	tagInfos.clear();
	
	findMuons(event, tightMuons, looseMuons, HT, qcdMode);
	if(tightMuons.size() > 2 || looseMuons.size() > 0) {
	  nCnt[23][qcdMode]++;
	  continue;
	}
	
	findElectrons(event, tightMuons, looseMuons, tightEles, looseEles, HT, qcdMode);
	if(tightEles.size() > 2 || looseEles.size() > 0) {
	  nCnt[29][qcdMode]++;
	  continue;
	}
	
	if(tightMuons.size() + tightEles.size() != 2) {
	  nCnt[24][qcdMode]++;
	  continue;
	}
	if(looseMuons.size() + looseEles.size() != 0) {
	  nCnt[26][qcdMode]++;
	  continue;
	}

	if(tightEles.size() != 2 && tightMuons.size() != 2) continue;
	
	bool passHLT = true;
	if(useTrigger) {
	  if(tightEles.size() == 2) passHLT = PassTriggers(1);

	  else if(tightMuons.size() == 2) {
	    if(qcdMode == kSignal) passHLT = PassTriggers(2);
	    if(kSignal == kMuonQCD) passHLT = PassTriggers(3);
	  }
	}
	if(!passHLT) {
	  nCnt[25][qcdMode]++;
	  continue;
	}
	
	findPhotons(event, 
		    photons,
		    tightMuons, looseMuons,
		    tightEles, looseEles,
		    HT,
		    photonMode);

	float HT_jets = 0.;
	TLorentzVector hadronicSystem(0., 0., 0., 0.);
	
	findJets(event, 
		 tightMuons, looseMuons,
		 tightEles, looseEles,
		 photons,
		 pfJets, btags,
		 sf,
		 tagInfos, csvValues, 
		 pfJets_corrP4, btags_corrP4, 
		 HT_jets, hadronicSystem);
	
	SetTreeValues_ZGamma(treeMap,
			     tightMuons, tightEles, 
			     pfJets, btags,
			     photons,
			     pfJets_corrP4, btags_corrP4,
			     csvValues,
			     hadronicSystem,
			     HT, HT_jets,
			     nPVertex,
			     0, 0, 0, 0,
			     jentry);
	
	////////////////////
	
	for(int chan = 0; chan < nChannels; chan++) {
	  
	  if(pfJets.size() < nJetReq[chan]) continue;
	  if((nBtagInclusive[chan] && btags.size() < nBtagReq[chan]) || (!nBtagInclusive[chan] && btags.size() != nBtagReq[chan])) continue;
	  
	  if(tightEles.size() != nEleReq[chan] * 2) continue;
	  if(tightMuons.size() != nMuonReq[chan] * 2) continue;
	  
	  if(photonMode == kSignalPhotons) {
	    if(qcdMode == kSignal) {
	      nCnt[2][chan]++;
	      signalTrees[chan]->Fill();
	    }
	    else if(qcdMode == kElectronQCD) {
	      nCnt[3][chan]++;
	      eQCDTrees[chan]->Fill();
	    }
	    else if(qcdMode == kMuonQCD) {
	      nCnt[4][chan]++;
	      muQCDTrees[chan]->Fill();
	    }
	  }
	  
	  if(photonMode == kFakePhotons) {
	    if(qcdMode == kSignal) {
	      nCnt[5][chan]++;
	      fakeTrees[chan]->Fill();
	    }
	    else if(qcdMode == kElectronQCD) {
	      nCnt[6][chan]++;
	      eQCDfakeTrees[chan]->Fill();
	    }
	    else if(qcdMode == kMuonQCD) {
	      nCnt[7][chan]++;
	      muQCDfakeTrees[chan]->Fill();
	    }
	  }

	} // loop over jet/btag req channels
    
	///////////////////////////////////
    
      } // for photon modes

    } // for qcd modes

    if(quitAfterProcessing) break;
  } // for entries
  
  cout << "-------------------Job Summary-----------------" << endl;
  cout << "Total_events         : " << nCnt[0][0] << endl;
  cout << "in_JSON              : " << nCnt[1][0] << endl;
  cout << "-----------------------------------------------" << endl;
  cout << endl;
  for(int i = 0; i < nChannels; i++) {
    cout << "--------------- " << channels[i] << " Requirement ----------------" << endl;
    cout << "Signal               " << channels[i] << " events : " << nCnt[2][i] << endl;
    cout << "eQCD                 " << channels[i] << " events : " << nCnt[3][i] << endl;
    cout << "muQCD                " << channels[i] << " events : " << nCnt[4][i] << endl;
    cout << "fake      " << channels[i] << " events : " << nCnt[5][i] << endl;
    cout << "eQCDfake  " << channels[i] << " events : " << nCnt[6][i] << endl;
    cout << "muQCDfake " << channels[i] << " events : " << nCnt[7][i] << endl;
  }
  cout << "-----------------------------------------------" << endl;
  cout << endl;
  cout << "----------------Continues, info----------------" << endl;
  cout << "fail MET filters         : " << nCnt[21][0] << endl;
  cout << "No primary vertex        : " << nCnt[22][0] << endl;
  cout << "Fail signal HLT          : " << nCnt[25][0] << endl;
  cout << "Fail eQCD HLT            : " << nCnt[25][1] << endl;
  cout << "Fail muQCD HLT           : " << nCnt[25][2] << endl;
  cout << "-----------------------------------------------" << endl;
  cout << endl;

  out->cd();
  out->Write();
  out->Close();

}

void SusyEventAnalyzer::ZGammaAcceptance() {

  const int NCNT = 50;
  int nCnt[NCNT][nChannels];
  for(int i = 0; i < NCNT; i++) {
    for(int j = 0; j < nChannels; j++) {
      nCnt[i][j] = 0;
    }
  }
  
  TString output_code_t = FormatName(scan);

  // open histogram file and define histograms
  TFile * out = new TFile("zgamma"+output_code_t+".root", "RECREATE");
  out->cd();

  TH1D * h_nEvents = new TH1D("nEvents"+output_code_t, "nEvents"+output_code_t, 1, 0, 1);

  TH2D * h_whizard_phaseSpace = new TH2D("whizard_phaseSpace"+output_code_t, "whizard_phaseSpace"+output_code_t, 500, 0, 1000, 500, 0, 5);
  TH2D * h_madgraph_phaseSpace = new TH2D("madgraph_phaseSpace"+output_code_t, "madgraph_phaseSpace"+output_code_t, 500, 0, 1000, 500, 0, 5);

  const int nTreeVariables = 101;

  TString varNames[nTreeVariables] = {
    "pfMET", "pfMET_x", "pfMET_y", "pfMET_phi",
    "pfMET_sysShift", "pfMET_sysShift_phi",
    "pfMET_t1", "pfMET_t1p2", "pfMET_t01", "pfMET_t01p2", "pfNoPUMET", "pfMVAMET", "genMET",
    "Njets", "Nbtags", "Nphotons",
    "Ngamma", "Nfake",
    "HT", "HT_jets", "hadronic_pt", 
    "ele1_pt", "ele1_phi", "ele1_eta", "ele1_mvaTrigV0", "ele1_relIso",
    "ele2_pt", "ele2_phi", "ele2_eta", "ele2_mvaTrigV0", "ele2_relIso",
    "muon1_pt", "muon1_phi", "muon1_eta", "muon1_relIso",
    "muon2_pt", "muon2_phi", "muon2_eta", "muon2_relIso",
    "z_mass", "z_pt", "z_eta", "z_phi",
    "leadPhotonEt", "leadPhotonEta", "leadPhotonPhi", "leadChargedHadronIso", "leadSigmaIetaIeta", "lead_nPixelSeeds", "leadMVAregEnergy", "leadMVAregErr",
    "leadNeutralHadronIso", "leadPhotonIso",
    "trailPhotonEt", "trailPhotonEta", "trailPhotonPhi", "trailChargedHadronIso", "trailSigmaIetaIeta", "trail_nPixelSeeds", "trailMVAregEnergy", "trailMVAregErr",
    "trailNeutralHadronIso", "trailPhotonIso",
    "photon_invmass", "photon_dR", "photon_dPhi", "diEMpT", "diJetPt",
    "mLepGammaLead", "mLepGammaTrail", "mLepGammaGamma",
    "jet1_pt", "jet2_pt", "jet3_pt", "jet4_pt",
    "btag1_pt", "btag2_pt",
    "max_csv", "submax_csv", "min_csv",
    "nPV",
    "pileupWeight", "pileupWeightErr", "pileupWeightUp", "pileupWeightDown",
    "btagWeight", "btagWeightUp", "btagWeightDown", "btagWeightErr",
    "metFilterBit",
    "ttbarDecayMode",
    "overlaps_whizard", "overlaps_madgraph",
    "TopPtReweighting", "TopPtReweighting_ttHbb",
    "leadMatchGamma", "leadMatchElectron", "leadMatchJet",
    "trailMatchGamma", "trailMatchElectron", "trailMatchJet"};
    
  map<TString, float> treeMap;
  for(int i = 0; i < nTreeVariables; i++) treeMap[varNames[i]] = 0.;

  vector<TTree*> signalTrees, signalTrees_JECup, signalTrees_JECdown;
  vector<TTree*> fakeTrees, fakeTrees_JECup, fakeTrees_JECdown;
  vector<TTree*> eQCDTrees, eQCDfakeTrees,
    muQCDTrees, muQCDfakeTrees;
  
  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_signalTree", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    signalTrees.push_back(tree);
  }
  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_signalTree_JECup", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    signalTrees_JECup.push_back(tree);
  }
  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_signalTree_JECdown", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    signalTrees_JECdown.push_back(tree);
  }

  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_fakeTree", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    fakeTrees.push_back(tree);
  }
  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_fakeTree_JECup", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    fakeTrees_JECup.push_back(tree);
  }
  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_fakeTree_JECdown", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    fakeTrees_JECdown.push_back(tree);
  }

  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_eQCDTree", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    eQCDTrees.push_back(tree);
  }
  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_eQCDfakeTree", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    eQCDfakeTrees.push_back(tree);
  }

  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_muQCDTree", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    muQCDTrees.push_back(tree);
  }
  for(int i = 0; i < nChannels; i++) {
    TTree * tree = new TTree(channels[i]+"_muQCDfakeTree", "An event tree for final analysis");
    for(int j = 0; j < nTreeVariables; j++) tree->Branch(varNames[j], &treeMap[varNames[j]], varNames[j]+"/F");
    muQCDfakeTrees.push_back(tree);
  }

  ScaleFactorInfo sf(btagger);
  TFile * btagEfficiency = new TFile("btagEfficiency"+output_code_t+".root", "READ");
  sf.SetTaggingEfficiencies((TH1F*)btagEfficiency->Get("lEff"+output_code_t), (TH1F*)btagEfficiency->Get("cEff"+output_code_t), (TH1F*)btagEfficiency->Get("bEff"+output_code_t));

  // get pileup weights
  TFile * puFile = new TFile("pileupReweighting"+output_code_t+".root", "READ");
  TH1F * puWeights = (TH1F*)puFile->Get("puWeights"+output_code_t);
  TH1F * puWeights_up = (TH1F*)puFile->Get("puWeights_up"+output_code_t);
  TH1F * puWeights_down = (TH1F*)puFile->Get("puWeights_down"+output_code_t);

  Long64_t nEntries = fTree->GetEntries();
  cout << "Total events in files : " << nEntries << endl;
  cout << "Events to be processed : " << processNEvents << endl;
  h_nEvents->Fill(0., (Double_t)nEntries);

  vector<susy::Muon*> tightMuons, looseMuons;
  vector<susy::Electron*> tightEles, looseEles;
  vector<susy::PFJet*> pfJets, btags;
  vector<TLorentzVector> pfJets_corrP4, btags_corrP4;
  vector<float> csvValues;
  vector<susy::Photon*> photons;
  vector<BtagInfo> tagInfos;

  // start event looping
  Long64_t jentry = 0;
  while(jentry != processNEvents && event.getEntry(jentry++) != 0) {

    if(printLevel > 0 || (printInterval > 0 && (jentry >= printInterval && jentry%printInterval == 0))) {
      cout << int(jentry) << " events processed with run = " << event.runNumber << ", event = " << event.eventNumber << endl;
    }

    nCnt[0][0]++; // events

    float numTrueInt = -1.;
    susy::PUSummaryInfoCollection::const_iterator iBX = event.pu.begin();
    bool foundInTimeBX = false;
    while((iBX != event.pu.end()) && !foundInTimeBX) {
      if(iBX->BX == 0) {
	numTrueInt = iBX->trueNumInteractions;
	foundInTimeBX = true;
      }
      ++iBX;
    }

    float eventWeight = 0.;
    float eventWeightErr = 0.;
    float eventWeightUp = 0.;
    float eventWeightDown = 0.;
    if(numTrueInt >= 0.) {
      int binNum = puWeights->GetXaxis()->FindBin(numTrueInt);
      eventWeight = puWeights->GetBinContent(binNum);
      eventWeightErr = puWeights->GetBinError(binNum);
      eventWeightUp = puWeights_up->GetBinContent(binNum);
      eventWeightDown = puWeights_down->GetBinContent(binNum);
    }

    if(!doPileupReweighting) {
      eventWeight = 1.;
      eventWeightErr = 0.;
      eventWeightUp = 1.;
      eventWeightDown = 1.;
    }

    int nPVertex = GetNumberPV(event);
    if(nPVertex == 0) continue;
    
    fill_whizard_phaseSpace(h_whizard_phaseSpace);
    fill_madgraph_phaseSpace(h_madgraph_phaseSpace);

    for(int qcdMode = kSignal; qcdMode < kNumSearchModes; qcdMode++) {
      for(int jetSyst = kCentral; jetSyst < kNumJetSytematics; jetSyst++) {
	for(int photonMode = kSignalPhotons; photonMode < kNumPhotonModes; photonMode++) {

	  if(qcdMode != kSignal && jetSyst != kCentral) continue;

	  float HT = 0.;
	  
	  tightMuons.clear();
	  looseMuons.clear();
	  tightEles.clear();
	  looseEles.clear();
	  pfJets.clear();
	  btags.clear();
	  pfJets_corrP4.clear();
	  btags_corrP4.clear();
	  csvValues.clear();
	  photons.clear();
	  tagInfos.clear();
	  
	  findMuons(event, tightMuons, looseMuons, HT, qcdMode);
	  if(tightMuons.size() > 2 || looseMuons.size() > 0) continue;
	  
	  findElectrons(event, tightMuons, looseMuons, tightEles, looseEles, HT, qcdMode);
	  if(tightEles.size() > 2 || looseEles.size() > 0) continue;
	  
	  if(tightMuons.size() + tightEles.size() != 2) continue;
	  if(looseMuons.size() + looseEles.size() != 0) continue;
	  
	  if(tightEles.size() != 2 && tightMuons.size() != 2) continue;

	  bool passHLT = true;
	  if(useTrigger) {
	    if(tightEles.size() == 2) passHLT = PassTriggers(1);

	    else if(tightMuons.size() == 2) {
	      if(qcdMode == kSignal) passHLT = PassTriggers(2);
	      if(kSignal == kMuonQCD) passHLT = PassTriggers(3);
	    }
	  }
	  if(!passHLT) continue;
	  
	  findPhotons(event, 
		      photons,
		      tightMuons, looseMuons,
		      tightEles, looseEles,
		      HT,
		      photonMode);

	  float HT_jets = 0.;
	  TLorentzVector hadronicSystem(0., 0., 0., 0.);
	  
	  findJets_inMC(event, 
			tightMuons, looseMuons,
			tightEles, looseEles,
			photons,
			pfJets, btags,
			sf,
			tagInfos, csvValues, 
			pfJets_corrP4, btags_corrP4, 
			HT_jets, hadronicSystem,
			jetSyst);
	  
	  float btagWeight[nChannels];
	  float btagWeightUp[nChannels];
	  float btagWeightDown[nChannels];
	  float btagWeightError[nChannels];
	  for(int chan = 0; chan < nChannels; chan++) {
	    BtagWeight * tagWeight = new BtagWeight(nBtagReq[chan]);
	    pair<float, float> weightResult = tagWeight->weight(tagInfos, btags.size(), 0., false, nBtagInclusive[chan]);
	    btagWeight[chan] = weightResult.first;
	    btagWeightError[chan] = weightResult.second;
	    
	    btagWeightUp[chan] = (tagWeight->weight(tagInfos, btags.size(), 1., true, nBtagInclusive[chan])).first;
	    btagWeightDown[chan] = (tagWeight->weight(tagInfos, btags.size(), -1., true, nBtagInclusive[chan])).first;
	    
	    delete tagWeight;
	  }
	  
	  SetTreeValues_ZGamma(treeMap,
			       tightMuons, tightEles, 
			       pfJets, btags,
			       photons,
			       pfJets_corrP4, btags_corrP4,
			       csvValues,
			       hadronicSystem,
			       HT, HT_jets,
			       nPVertex,
			       eventWeight, eventWeightErr, eventWeightUp, eventWeightDown,
			       0);
	  
	  ////////////////////
	  
	  for(int chan = 0; chan < nChannels; chan++) {
	    
	    if(pfJets.size() < nJetReq[chan]) continue;
	    if((nBtagInclusive[chan] && btags.size() < nBtagReq[chan]) || (!nBtagInclusive[chan] && btags.size() != nBtagReq[chan])) continue;
	    
	    if(tightEles.size() != nEleReq[chan] * 2) continue;
	    if(tightMuons.size() != nMuonReq[chan] * 2) continue;
	    
	    treeMap["btagWeight"] = btagWeight[chan];
	    treeMap["btagWeightErr"] = btagWeightError[chan];
	    treeMap["btagWeightUp"] = btagWeightUp[chan];
	    treeMap["btagWeightDown"] = btagWeightDown[chan];

	    if(photonMode == kSignalPhotons) {
	      if(qcdMode == kSignal) {
		
		if(jetSyst == kCentral) {
		  nCnt[2][chan]++;
		  signalTrees[chan]->Fill();
		}
		else if(jetSyst == kJECup) signalTrees_JECup[chan]->Fill();
		else if(jetSyst == kJECdown) signalTrees_JECdown[chan]->Fill();

	      }
	      else if(qcdMode == kElectronQCD) {
		nCnt[3][chan]++;
		eQCDTrees[chan]->Fill();
	      }
	      else if(qcdMode == kMuonQCD) {
		nCnt[4][chan]++;
		muQCDTrees[chan]->Fill();
	      }
	    }
	    
	    if(photonMode == kFakePhotons) {
	      if(qcdMode == kSignal) {
		if(jetSyst == kCentral) {
		  nCnt[5][chan]++;
		  fakeTrees[chan]->Fill();
		}
		else if(jetSyst == kJECup) fakeTrees_JECup[chan]->Fill();
		else if(jetSyst == kJECdown) fakeTrees_JECdown[chan]->Fill();
	      }
	      else if(qcdMode == kElectronQCD && jetSyst == kCentral) {
		nCnt[6][chan]++;
		eQCDfakeTrees[chan]->Fill();
	      }
	      else if(qcdMode == kMuonQCD && jetSyst == kCentral) {
		nCnt[7][chan]++;
		muQCDfakeTrees[chan]->Fill();
	      }
	    }

	  } // for channels

	} // for photon modes
	  
      } // for jet systematic modes
	
    } // for qcd modes

  } // for entries

  cout << "-------------------Job Summary-----------------" << endl;
  cout << "Total_events         : " << nCnt[0][0] << endl;
  cout << "-----------------------------------------------" << endl;
  cout << endl;
  for(int i = 0; i < nChannels; i++) {
    cout << "---------------- " << channels[i] << " Requirement ----------------" << endl;
    cout << "Signal               " << channels[i] << " events : " << nCnt[2][i] << endl;
    cout << "eQCD                 " << channels[i] << " events : " << nCnt[3][i] << endl;
    cout << "muQCD                " << channels[i] << " events : " << nCnt[4][i] << endl;
    cout << "fake                 " << channels[i] << " events : " << nCnt[5][i] << endl;
    cout << "eQCDfake             " << channels[i] << " events : " << nCnt[6][i] << endl;
    cout << "muQCDfake            " << channels[i] << " events : " << nCnt[7][i] << endl;
  }
  cout << endl;
  cout << "----------------Continues, info----------------" << endl;
 
  puFile->Close();
  btagEfficiency->Close();

  out->Write();
  out->Close();

}

void SusyEventAnalyzer::CutFlowData() {

  TFile* out = new TFile("cutflow_"+outputName+"_"+btagger+".root", "RECREATE");
  out->cd();

  const int nCuts = 14;
  TString cutNames[nCuts] = {
    "All events",
    "JSON",
    "MET filters",
    "nPV #geq 1",
    "== 1 tight lepton",
    "==0 loose leptons",
    "HLT",
    "nJets #geq 3",
    "nBtags #geq 1",
    "N(#gamma, f) #geq 1",
    "SR1",
    "CR1",
    "SR2",
    "CR2"};
    
  TH1D * h_cutflow_ele = new TH1D("cutflow_ele", "cutflow_ele", nCuts, 0, nCuts);
  TH1D * h_cutflow_muon = new TH1D("cutflow_muon", "cutflow_muon", nCuts, 0, nCuts);

  for(int i = 0; i < nCuts; i++) {
    h_cutflow_ele->GetXaxis()->SetBinLabel(i+1, cutNames[i]);
    h_cutflow_muon->GetXaxis()->SetBinLabel(i+1, cutNames[i]);
  }
 
  ScaleFactorInfo sf(btagger);

  Long64_t nEntries = fTree->GetEntries();
  cout << "Total events in files : " << nEntries << endl;
  cout << "Events to be processed : " << processNEvents << endl;

  vector<susy::Muon*> tightMuons, looseMuons;
  vector<susy::Electron*> tightEles, looseEles;
  vector<susy::PFJet*> pfJets, btags;
  vector<TLorentzVector> pfJets_corrP4, btags_corrP4;
  vector<float> csvValues;
  vector<susy::Photon*> photons;
  vector<BtagInfo> tagInfos;

  // start event looping
  Long64_t jentry = 0;
  while(jentry != processNEvents && event.getEntry(jentry++) != 0) {

    if(printLevel > 0 || (printInterval > 0 && (jentry >= printInterval && jentry%printInterval == 0))) {
      cout << int(jentry) << " events processed with run = " << event.runNumber << ", event = " << event.eventNumber << endl;
    }

    // All events
    h_cutflow_ele->Fill(0);
    h_cutflow_muon->Fill(0);
    
    if(useJson && event.isRealData && !IsGoodLumi(event.runNumber, event.luminosityBlockNumber)) continue;
    // JSON
    h_cutflow_ele->Fill(1);
    h_cutflow_muon->Fill(1);

    if(event.isRealData) {
      if(event.passMetFilters() != 1 ||
	 event.passMetFilter(susy::kEcalLaserCorr) != 1 ||
	 event.passMetFilter(susy::kManyStripClus53X) != 1 ||
	 event.passMetFilter(susy::kTooManyStripClus53X) != 1) {
	continue;
      }
    }
    // MET filters
    h_cutflow_ele->Fill(2);
    h_cutflow_muon->Fill(2);

    int nPVertex = GetNumberPV(event);
    if(nPVertex == 0) {
      continue;
    }

    // nPV #geq 1
    h_cutflow_ele->Fill(3);
    h_cutflow_muon->Fill(3);

    int qcdMode = kSignal;
    int photonMode = kSignalPhotons;
    
    float HT = 0.;
	
    tightMuons.clear();
    looseMuons.clear();
    tightEles.clear();
    looseEles.clear();
    pfJets.clear();
    btags.clear();
    pfJets_corrP4.clear();
    btags_corrP4.clear();
    csvValues.clear();
    photons.clear();
    tagInfos.clear();
	
    findMuons(event, tightMuons, looseMuons, HT, qcdMode);
    if(tightMuons.size() > 1 || looseMuons.size() > 0) continue;
    
    findElectrons(event, tightMuons, looseMuons, tightEles, looseEles, HT, qcdMode);
    if(tightEles.size() > 1 || looseEles.size() > 0) continue;
    
    if(tightMuons.size() + tightEles.size() != 1) continue;

    bool isEleEvent = (tightEles.size() == 1);

    // == 1 tight lepton
    if(isEleEvent) h_cutflow_ele->Fill(4);
    else h_cutflow_muon->Fill(4);

    if(looseMuons.size() + looseEles.size() != 0) continue;
    // == 0 loose leptons
    if(isEleEvent) h_cutflow_ele->Fill(5);
    else h_cutflow_muon->Fill(5);
    
    bool passHLT = true;
    if(useTrigger) {
      if(tightEles.size() == 1) passHLT = PassTriggers(1);
      
      else if(tightMuons.size() == 1) {
	if(qcdMode == kSignal) passHLT = PassTriggers(2);
	if(kSignal == kMuonQCD) passHLT = PassTriggers(3);
      }
    }
    if(!passHLT) continue;
    // HLT
    if(isEleEvent) h_cutflow_ele->Fill(6);
    else h_cutflow_muon->Fill(6);
    
    findPhotons(event, 
		photons,
		tightMuons, looseMuons,
		tightEles, looseEles,
		HT,
		photonMode);

    float HT_jets = 0.;
    TLorentzVector hadronicSystem(0., 0., 0., 0.);
    
    findJets(event, 
	     tightMuons, looseMuons,
	     tightEles, looseEles,
	     photons,
	     pfJets, btags,
	     sf,
	     tagInfos, csvValues, 
	     pfJets_corrP4, btags_corrP4, 
	     HT_jets, hadronicSystem);
    
    ////////////////////

    if(pfJets.size() < 3) continue;
    // nJets #geq 3
    if(isEleEvent) h_cutflow_ele->Fill(7);
    else h_cutflow_muon->Fill(7);
    
    if(btags.size() < 1) continue;
    // nBtags #geq 1
    if(isEleEvent) h_cutflow_ele->Fill(8);
    else h_cutflow_muon->Fill(8);

    if(photons.size() > 0) {
      if(isEleEvent) h_cutflow_ele->Fill(9);
      else h_cutflow_muon->Fill(9);
    }
    
  } // for entries
  
  out->cd();
  out->Write();
  out->Close();

}

void SusyEventAnalyzer::CutFlowMC() {

  TString output_code_t = FormatName(scan);

  // open histogram file and define histograms
  TFile * out = new TFile("cutflow"+output_code_t+".root", "RECREATE");
  out->cd();

  const int nCuts = 14;
  TString cutNames[nCuts] = {
    "All events",
    "JSON",
    "MET filters",
    "nPV #geq 1",
    "== 1 tight lepton",
    "==0 loose leptons",
    "HLT",
    "nJets #geq 3",
    "nBtags #geq 1",
    "N(#gamma, f) #geq 1",
    "SR1",
    "CR1",
    "SR2",
    "CR2"};

  const int nPhotonCuts = 11;
  TString photonCutNames[nPhotonCuts] = {
    "All candidates",
    "|#eta| < 1.4442",
    "E_{T} > 20 GeV",
    "H/E < 0.05",
    "passElectronVeto",
    "neutralHadIso",
    "photonIso",
    "chHadIso",
    "sIetaIeta",
    "#Delta R(#gamma, #mu) #geq 0.7",
    "#Delta R(#gamma, ele) #geq 0.7"};
    
  TH1D * h_cutflow_ele = new TH1D("cutflow_ele", "cutflow_ele", nCuts, 0, nCuts);
  TH1D * h_cutflow_muon = new TH1D("cutflow_muon", "cutflow_muon", nCuts, 0, nCuts);

  TH1D * h_cutflow_photons = new TH1D("cutflow_photons", "cutflow_photons", nPhotonCuts, 0, nPhotonCuts);
  
  for(int i = 0; i < nCuts; i++) {
    h_cutflow_ele->GetXaxis()->SetBinLabel(i+1, cutNames[i]);
    h_cutflow_muon->GetXaxis()->SetBinLabel(i+1, cutNames[i]);
  }

  for(int i = 0; i < nPhotonCuts; i++) h_cutflow_photons->GetXaxis()->SetBinLabel(i+1, photonCutNames[i]);
  
  ScaleFactorInfo sf(btagger);

  Long64_t nEntries = fTree->GetEntries();
  cout << "Total events in files : " << nEntries << endl;
  cout << "Events to be processed : " << processNEvents << endl;

  vector<susy::Muon*> tightMuons, looseMuons;
  vector<susy::Electron*> tightEles, looseEles;
  vector<susy::PFJet*> pfJets, btags;
  vector<TLorentzVector> pfJets_corrP4, btags_corrP4;
  vector<float> csvValues;
  vector<susy::Photon*> photons;
  vector<BtagInfo> tagInfos;

  // start event looping
  Long64_t jentry = 0;
  while(jentry != processNEvents && event.getEntry(jentry++) != 0) {

    if(printLevel > 0 || (printInterval > 0 && (jentry >= printInterval && jentry%printInterval == 0))) {
      cout << int(jentry) << " events processed with run = " << event.runNumber << ", event = " << event.eventNumber << endl;
    }

    // All events
    h_cutflow_ele->Fill(0);
    h_cutflow_muon->Fill(0);

    // JSON
    h_cutflow_ele->Fill(1);
    h_cutflow_muon->Fill(1);
    
    // MET filters
    h_cutflow_ele->Fill(2);
    h_cutflow_muon->Fill(2);
    
    int nPVertex = GetNumberPV(event);
    if(nPVertex == 0) continue;

    // nPV #geq 1
    h_cutflow_ele->Fill(3);
    h_cutflow_muon->Fill(3);
				 
    int qcdMode = kSignal;
    int jetSyst = kCentral;
    int photonMode = kSignalPhotons;
    
    float HT = 0.;
    
    tightMuons.clear();
    looseMuons.clear();
    tightEles.clear();
    looseEles.clear();
    pfJets.clear();
    btags.clear();
    pfJets_corrP4.clear();
    btags_corrP4.clear();
    csvValues.clear();
    photons.clear();
    tagInfos.clear();
    
    findMuons(event, tightMuons, looseMuons, HT, qcdMode);
    if(tightMuons.size() > 1 || looseMuons.size() > 0) continue;
    
    findElectrons(event, tightMuons, looseMuons, tightEles, looseEles, HT, qcdMode);
    if(tightEles.size() > 1 || looseEles.size() > 0) continue;
    
    if(tightMuons.size() + tightEles.size() != 1) continue;

    bool isEleEvent = (tightEles.size() == 1);

    // == 1 tight lepton
    if(isEleEvent) h_cutflow_ele->Fill(4);
    else h_cutflow_muon->Fill(4);


    if(looseMuons.size() + looseEles.size() != 0) continue;
    // == 0 loose leptons
    if(isEleEvent) h_cutflow_ele->Fill(5);
    else h_cutflow_muon->Fill(5);
    
    bool passHLT = true;
    if(useTrigger) {
      if(tightEles.size() == 1) passHLT = PassTriggers(1);
      
      else if(tightMuons.size() == 1) {
	if(qcdMode == kSignal) passHLT = PassTriggers(2);
	if(kSignal == kMuonQCD) passHLT = PassTriggers(3);
      }
    }
    if(!passHLT) continue;
    // HLT
    if(isEleEvent) h_cutflow_ele->Fill(6);
    else h_cutflow_muon->Fill(6);

    findPhotonsWithCutflow(event, 
			   photons,
			   tightMuons, looseMuons,
			   tightEles, looseEles,
			   HT,
			   h_cutflow_photons);
    
    float HT_jets = 0.;
    TLorentzVector hadronicSystem(0., 0., 0., 0.);
    
    findJets_inMC(event, 
		  tightMuons, looseMuons,
		  tightEles, looseEles,
		  photons,
		  pfJets, btags,
		  sf,
		  tagInfos, csvValues, 
		  pfJets_corrP4, btags_corrP4, 
		  HT_jets, hadronicSystem,
		  jetSyst);
    
    ////////////////////

    if(pfJets.size() < 3) continue;
    // nJets #geq 3
    if(isEleEvent) h_cutflow_ele->Fill(7);
    else h_cutflow_muon->Fill(7);
    
    if(btags.size() < 1) continue;
    // nBtags #geq 1
    if(isEleEvent) h_cutflow_ele->Fill(8);
    else h_cutflow_muon->Fill(8);

    if(photons.size() > 0) {
      if(isEleEvent) h_cutflow_ele->Fill(9);
      else h_cutflow_muon->Fill(9);
    }

  } // for entries

  out->Write();
  out->Close();

}
