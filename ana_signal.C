void ana_signal(TString scan = "stop-bino", TString discriminant = "CSVM", bool isMC = true, bool isFastSim = true) {

  gROOT->Reset();
  gSystem->Load("libSusyEvent.so");
  
  gROOT->LoadMacro("SusyEventAnalyzer.cc+");

  TChain chain("susyTree");

  char* tmp = getenv("CONDOR_SECTION");
  cout << "tmp = " << tmp << endl;
  int index = atoi ( tmp );
  cout << "index = " << index << endl;

  //int index1 = int(index)/17*100 + 400;
  //int index2 = int(index)%17*100 + 420;

  Double_t mst[29] = {110, 160, 185, 210, 235, 260, 285, 310, 335, 360, 385, 410, 460, 510, 560, 610, 660, 710, 810, 910, 1010, 1110, 1210, 1310, 1410, 1510, 1710, 2010, 5010};
  Double_t mGluino = 5050;
  Double_t mBino[31] = {25, 50, 75, 100, 125, 150, 175, 200, 225, 250, 275, 300, 325, 375, 425, 475, 525, 575, 625, 675, 725, 825, 925, 1025, 1125, 1225, 1325, 1425, 1525, 1725, 2025};

  Double_t mst_ext[22] = {100, 150, 200, 250, 300, 350, 400, 450, 500, 550, 600, 650, 700, 750, 800, 850, 900, 950, 1000, 1500, 2000, 5000};
  Double_t mGluino_ext = 5025;
  Double_t mBino_ext[5] = {25, 50, 75, 100, 125};

  // stop-bino scan
  int index1 = 0;
  int index2 = 0;

  if(scan == "stop-bino") {
    index1 = mst[int(index)/31];
    index2 = mBino[int(index)%31];
  }
  else if(scan == "stop-bino extended") {
    index1 = mst_ext[int(index)/5];
    index2 = mBino_ext[int(index)%5];
  }

  cout << "index1 = " << index1 << endl;
  cout << "index2 = " << index2 << endl;

  char input_file[500];

  if(scan == "stop-bino") sprintf(input_file, "dcap:///pnfs/cms/WAX/resilient/bfrancis/SusyNtuples/cms538v1/naturalBinoNLSP/tree_naturalBinoNLSPout_mst_%d_M3_5050_M1_%d.root", index1, index2);
  else if(scan == "stop-bino extended") sprintf(input_file, "/eos/uscms/store/user/lpcpjm/PrivateMC/FastSim/533p3_full/naturalBinoNLSP_try3/SusyNtuple/cms533v1_v1/tree_naturalBinoNLSPout_mst_%d_M3_5025_M1_%d.root", index1, index2);

  cout << "input_file = " << input_file << endl;

  chain.Add(input_file);

  chain.SetBranchStatus("*", 1);

  if(chain.LoadTree(0) != 0) {
    cerr << "Error with input chain. Do the files exist?" << endl;
    return;
  }

  SusyEventAnalyzer * sea = new SusyEventAnalyzer(chain);
  sea->SetUseDPhiCut(false);

  sea->SetScanName(scan);
  sea->SetPrintInterval(1e4);
  sea->SetPrintLevel(0);
  sea->SetUseTrigger(true);

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
  qcd_mu_trigger.push_back("HLT_Mu24_eta2p1_v");
  std::vector<int> qcd_mu_type;
  qcd_mu_type.push_back(3);
  sea->AddHlt(qcd_mu_trigger, qcd_mu_type);
  
  sea->SetProcessNEvents(-1);

  sea->SetUseJson(false);

  sea->SetIsMC(isMC);
  sea->SetIsFastSim(isFastSim);
  sea->SetDoBtagScaling(false);
  sea->SetBtagTechnicalStop("ABCD");

  sea->SetRejectFakeElectrons(false);

  sea->SetUseSyncFile(false);
  //sea->IncludeSyncFile("utility/syncList.txt");
  sea->SetCheckSingleEvent(false);
  sea->AddCheckSingleEvent(166890, 399, 436836287);

  sea->SetBtagger(discriminant);

  sea->AddValidTagger("TCHPT");
  sea->AddValidTagger("JPL");
  sea->AddValidTagger("JPM");
  sea->AddValidTagger("JPT");
  sea->AddValidTagger("CSVL");
  sea->AddValidTagger("CSVM");
  sea->AddValidTagger("CSVT");

  sea->SetDoPileupReweighting(true);

  TStopwatch ts;

  ts.Start();

  std::cout << std::endl << "PileupWeights()" << std::endl;
  sea->PileupWeights("pileup_22Jan2013_69400.root", "pileup_22Jan2013_72870.root", "pileup_22Jan2013_65930.root");
  std::cout << std::endl << "CalculateBtagEfficiency()" << std::endl;
  sea->CalculateBtagEfficiency();
  std::cout << std::endl << "Acceptance()" << std::endl;
  sea->Acceptance();

  ts.Stop();

  std::cout << "RealTime : " << ts.RealTime()/60.0 << " minutes" << std::endl;
  std::cout << "CPUTime  : " << ts.CpuTime()/60.0 << " minutes" << std::endl;

}

