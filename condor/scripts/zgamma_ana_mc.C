void zgamma_ana_mc(TString scan = "DATASETNAME", TString discriminant = "CSVM", bool isMC = true, bool isFastSim = false) {

  gROOT->Reset();
  gSystem->Load("libSusyEvent.so");

  gSystem->Load("SusyEventAnalyzer_cc.so");

  char* tmp = getenv("CONDOR_SECTION");
  int index = atoi ( tmp );
  cout << "index = " << index << endl;

  TChain chain("susyTree");

  

  chain.SetBranchStatus("*", 1);

  if(chain.LoadTree(0) != 0) {
    cerr << "Error with input chain. Do the files exist?" << endl;
    return;
  }

  SusyEventAnalyzer* sea = new SusyEventAnalyzer(chain);
  sea->SetUseDPhiCut(false);

  // configuration parameters
  // any values given here will replace the default values
  sea->SetPrintInterval(1e4);             // print frequency
  sea->SetPrintLevel(0);                  // print level for event contents

  std::vector<TString> ele_trigger;
  ele_trigger.push_back("HLT_Ele27_WP80_v");
  std::vector<int> ele_type;
  ele_type.push_back(1);
  sea->AddHlt(ele_trigger, ele_type);

  std::vector<TString> mu_trigger;
  mu_trigger.push_back("HLT_IsoMu24_eta2p1_v");
  std::vector<int> mu_type;
  mu_type.push_back(2);
  sea->AddHlt(mu_trigger, mu_type);

  std::vector<TString> qcd_mu_trigger;
  qcd_mu_trigger.push_back("HLT_IsoMu24_eta2p1_v");
  qcd_mu_trigger.push_back("HLT_Mu24_eta2p1_v");
  std::vector<int> qcd_mu_type;
  qcd_mu_type.push_back(3);
  qcd_mu_type.push_back(3);
  sea->AddHlt(qcd_mu_trigger, qcd_mu_type);

  sea->SetUseTrigger(true);

  sea->SetProcessNEvents(-1);      	  // number of events to be processed
  
  sea->SetIsMC(isMC);
  sea->SetIsFastSim(isFastSim);
  sea->SetScanName(scan);
  sea->SetDoPileupReweighting(true);
  sea->SetUseJson(false);
  sea->SetDoBtagScaling(false);
  sea->SetBtagTechnicalStop("ABCD");  

  sea->SetRejectFakeElectrons(false);

  sea->SetUseSyncFile(false);
  //sea->IncludeSyncFile("synchro/dmorse_ff.txt_not_brian_ff_nojet.txt");
  sea->SetCheckSingleEvent(false);
  sea->AddCheckSingleEvent(196203, 33, 27883630);

  sea->SetBtagger(discriminant);

  TString stage = "STAGING";

  if(stage == "pileup") {
    std::cout << std::endl << "PileupWeights()" << std::endl;
    sea->PileupWeights("pileup_22Jan2013_69400.root", "pileup_22Jan2013_72870.root", "pileup_22Jan2013_65930.root");
  }
  else if(stage == "btag") {
    std::cout << std::endl << "CalculateBtagEfficiency()" << std::endl;
    sea->CalculateBtagEfficiency();
  }
  else if(stage == "acceptance") {
    std::cout << std::endl << "ZGammaAcceptance()" << std::endl;
    sea->ZGammaAcceptance();
  }
  else if(stage == "all") {
    std::cout << std::endl << "PileupWeights()" << std::endl;
    sea->PileupWeights("pileup_22Jan2013_69400.root", "pileup_22Jan2013_72870.root", "pileup_22Jan2013_65930.root");
    std::cout << std::endl << "CalculateBtagEfficiency()" << std::endl;
    sea->CalculateBtagEfficiency();
    std::cout << std::endl << "ZGammaAcceptance()" << std::endl;
    sea->ZGammaAcceptance();
  }
  
}
