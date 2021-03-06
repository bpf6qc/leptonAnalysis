#include "CreatePlots.h"

using namespace std;

void readFitResults(TString fileName, vector<Float_t>& scales, vector<Float_t>& errors) {

  scales.clear();
  errors.clear();

  double sf, sfError;
  string dummy;

  ifstream input;
  input.open(fileName.Data());

  // skip the first line
  getline(input, dummy);

  while(1) {
    input >> dummy >> sf >> sfError;
    if(!input.good()) break;
    scales.push_back(sf);
    errors.push_back(sfError);
  }

  input.close();

}

void CombineFitResults(TString fileName, vector<Float_t>& scales, vector<Float_t>& errors) {

  vector<Float_t> newScales, newErrors;
  readFitResults(fileName, newScales, newErrors);

  for(unsigned int i = 0; i < scales.size(); i++) {
    Float_t a = scales[i];
    Float_t b = newScales[i];

    Float_t sa = errors[i];
    Float_t sb = newErrors[i];

    if(a == 0. || b == 0.) continue;

    Float_t val = a*b;
    Float_t err = a*b*sqrt(sa*sa/a/a + sb*sb/b/b);

    scales[i] = val;
    errors[i] = err;
  }

}

void CreatePlots(int channel, int controlRegion, bool needsQCD, TString metType, bool useWhizard, bool usePurityScaleFactors) {

  gROOT->Reset();
  gROOT->SetBatch(true);
  gROOT->SetStyle("Plain");
  gStyle->SetOptStat(0000);
  gStyle->SetOptTitle(0);

  vector<Float_t> sf_qcd, sf_ttjets, sf_wjets, sf_zjets, sf_singleTop, sf_diboson, sf_vgamma, sf_ttW, sf_ttZ, sf_ttgamma;
  vector<Float_t> sfError_qcd, sfError_ttjets, sfError_wjets, sfError_zjets, sfError_singleTop, sfError_diboson, sfError_vgamma, sfError_ttW, sfError_ttZ, sfError_ttgamma;

  // qcd
  if(controlRegion == kAny && channels[channel].Contains("bjj")) readFitResults("scaleFactors/qcdSF_kAny_"+channels[channel]+".txt", sf_qcd, sfError_qcd);

  // ttjets
  if(channels[channel].Contains("bjj")) readFitResults("scaleFactors/ttbarSF_M3_"+channels[channel]+".txt", sf_ttjets, sfError_ttjets);
  if(usePurityScaleFactors) {
    if(controlRegion == kSR1 && channels[channel].Contains("bjj")) {
      CombineFitResults("scaleFactors/puritySF_ttjets_"+channels[channel]+"_chHadIso_metCut_50.txt", sf_ttjets, sfError_ttjets);
    }
    if(controlRegion == kSR2 && channels[channel].Contains("bjj")) {
      CombineFitResults("scaleFactors/puritySF_ttjets_"+channels[channel]+"_chHadIso_metCut_50.txt", sf_ttjets, sfError_ttjets);
      CombineFitResults("scaleFactors/puritySF_ttjets_"+channels[channel]+"_chHadIso_metCut_50.txt", sf_ttjets, sfError_ttjets);
    }
  }
  
  // wjets
  if(channels[channel].Contains("bjj")) readFitResults("scaleFactors/wjetsSF_"+channels[channel]+".txt", sf_wjets, sfError_wjets);

  // zjets
  readFitResults("scaleFactors/dilepSF_"+channels[channel]+".txt", sf_zjets, sfError_zjets);
  if(channels[channel].Contains("ele") && controlRegion != kAny && controlRegion != kCR0) {
    CombineFitResults("scaleFactors/eleFakeRateSF_"+channels[channel]+".txt", sf_zjets, sfError_zjets);
  }

  // singleTop

  // diboson

  // vgamma
  readFitResults("scaleFactors/dilepSF_"+channels[channel]+".txt", sf_vgamma, sfError_vgamma);
  if(channels[channel].Contains("ele") && controlRegion != kAny && controlRegion != kCR0) {
    CombineFitResults("scaleFactors/eleFakeRateSF_"+channels[channel]+".txt", sf_vgamma, sfError_vgamma);
  }

  // ttW

  // ttZ

  // ttgamma
  if(usePurityScaleFactors) {
    if(controlRegion == kSR1 && channels[channel].Contains("bjj")) {
      readFitResults("scaleFactors/puritySF_ttgamma_"+channels[channel]+"_chHadIso_metCut_50.txt", sf_ttgamma, sfError_ttgamma);
    }
    if(controlRegion == kSR2 && channels[channel].Contains("bjj")) {
      readFitResults("scaleFactors/puritySF_ttgamma_"+channels[channel]+"_chHadIso_metCut_50.txt", sf_ttgamma, sfError_ttgamma);
      CombineFitResults("scaleFactors/puritySF_ttgamma_"+channels[channel]+"_chHadIso_metCut_50.txt", sf_ttgamma, sfError_ttgamma);
    }
  }

  PlotMaker * pMaker = new PlotMaker(channel, controlRegion, needsQCD, metType, sf_qcd, sfError_qcd);

  vector<TString> ttJets;
  ttJets.push_back("ttJetsSemiLep");
  ttJets.push_back("ttJetsFullLep");
  ttJets.push_back("ttJetsHadronic");
  pMaker->BookMCLayer(ttJets, kGray, "ttjets", "t#bar{t} + Jets", kGG, kTTbar, sf_ttjets, sfError_ttjets);

  vector<TString> wJets;
  //wJets.push_back("W1JetsToLNu");
  //wJets.push_back("W2JetsToLNu");
  wJets.push_back("W3JetsToLNu");
  wJets.push_back("W4JetsToLNu");
  pMaker->BookMCLayer(wJets, kOrange-3, "wjets", "W + Jets", kQQ, kV, sf_wjets, sfError_wjets);

  vector<TString> zJets;
  //zJets.push_back("dyJetsToLL");
  zJets.push_back("dy1JetsToLL");
  zJets.push_back("dy2JetsToLL");
  zJets.push_back("dy3JetsToLL");
  zJets.push_back("dy4JetsToLL");
  pMaker->BookMCLayer(zJets, kYellow, "zjets", "Z/#gamma* + Jets", kQQ, kV, sf_zjets, sfError_zjets);

  vector<TString> singleTop;
  singleTop.push_back("TBar_s");
  singleTop.push_back("TBar_t");
  singleTop.push_back("TBar_tW");
  singleTop.push_back("T_s");
  singleTop.push_back("T_t");
  singleTop.push_back("T_tW");
  pMaker->BookMCLayer(singleTop, kRed, "singleTop", "Single top", kQG, kTTbar, sf_singleTop, sfError_singleTop);

  vector<TString> diboson;
  diboson.push_back("WW");
  diboson.push_back("WZ");
  diboson.push_back("ZZ");
  pMaker->BookMCLayer(diboson, kViolet-2, "diboson", "VV, V#gamma", kQQ, kVV, sf_diboson, sfError_diboson);

  vector<TString> vgamma;
  vgamma.push_back("WGToLNuG");
  vgamma.push_back("ZGToLLG");
  pMaker->BookMCLayer(vgamma, kAzure-2, "vgamma", "VV, V#gamma", kQQ, kVV, sf_vgamma, sfError_vgamma);

  vector<TString>  ttW;
  ttW.push_back("TTWJets");
  pMaker->BookMCLayer(ttW, kCyan, "ttW", "t#bar{t} + V", kQQ, kTTbar, sf_ttW, sfError_ttW);

  vector<TString>  ttZ;
  ttZ.push_back("TTZJets");
  pMaker->BookMCLayer(ttZ, kOrange-5, "ttZ", "t#bar{t} + V", kGG, kTTbar, sf_ttZ, sfError_ttZ);

  vector<TString> ttgamma;
  if(useWhizard) ttgamma.push_back("ttA_2to5");
  else ttgamma.push_back("TTGamma");
  pMaker->BookMCLayer(ttgamma, 8, "ttgamma", "t#bar{t} + #gamma", kGG, kTTbar, sf_ttgamma, sfError_ttgamma);

  ///////////////////////////////////////////////////////

  bool usePasStyle = true;
  
  // aps15
  if(controlRegion == kAny) {
    pMaker->BookPlot(metType, true,
		     "#slash{E}_{T} (GeV)", "Number of Events / GeV",
		     0., 300., 7.e-3, 2.5e6,
		     0., 1.9,
		     true, true, true, usePasStyle);
  }
  else if(controlRegion == kCR1) {
    pMaker->BookPlot(metType, true,
		     "#slash{E}_{T} (GeV)", "Number of Events / GeV",
		     0., 800., 2.e-4, 1.e2,
		     0.35, 1.65,
		     false, true, true, usePasStyle);
  }
  else if(controlRegion == kCR2) {
    pMaker->BookPlot(metType, true,
		     "#slash{E}_{T} (GeV)", "Number of Events / GeV",
		     0., 799.9, 3.e-4, 40.0,
		     0., 1.9,
		     false, true, true, usePasStyle);
  }
  else if(controlRegion == kSR1) {
    pMaker->BookPlot(metType, true,
		     "#slash{E}_{T} (GeV)", "Number of Events / GeV",
		     0., 799.9, 7.e-4, 1.e2,
		     0.45, 1.55,
		     true, true, true, usePasStyle);
  }
  else if(controlRegion == kSR2) {
    pMaker->BookPlot(metType, true,
		     "#slash{E}_{T} (GeV)", "Number of Events / GeV",
		     0., 799.9, 3.e-4, 2.0,
		     0., 5.9,
		     true, true, true, usePasStyle);
  }
  else {
    pMaker->BookPlot(metType, true,
		     "#slash{E}_{T} (GeV)", "Number of Events / GeV",
		     0., 300., 7.e-3, 2.5e5,
		     0., 1.9,
		     true, true, true, usePasStyle);
  }
  
  //if(controlRegion == kCR1) pMaker->SetDoRebinMET(true);

  pMaker->BookPlot("m3", false,
		   "M3 (GeV/c^{2})", "Number of Events / GeV",
		   0., 300., 7.e-5, 2.e5,
		   0., 1.9,
		   true, true, true);

  // aps15
  if(controlRegion == kAny) {
    pMaker->BookPlot("Njets", false,
		     "Njets", "Number of Events",
		     2., 11., 2.e-2, 1.1e8,
		     0., 1.9,
		     true, true, true);
  }
  else{
    pMaker->BookPlot("Njets", false,
		     "Njets", "Number of Events",
		     2., 14., 2.e-3, 9.e5,
		     0., 1.9,
		     true, true, true);
  }

  pMaker->BookPlot("Nbtags", false,
		   "Nbtags", "Number of Events",
		   0., 8., 2.e-3, 9.e5,
		   0., 1.9,
		   true, true, true);

  pMaker->BookPlot("HT_jets", true,
		   "HT_jets", "Number of Events / GeV",
		   0., 1200., 2.e-4, 4.e3,
		   0., 1.9,
		   true, true, true);

  pMaker->BookPlot("hadronic_pt", true, 
		   "MHT (GeV/c)", "Number of Events / GeV",
		   0, 1500, 2.e-5, 8.e3,
		   0., 2.1,
		   true, true, true);

  pMaker->BookPlot("HT", true,
		   "HT (GeV)", "Number of Events / GeV",
		   0, 2000, 2.e-4, 8.e3,
		   0., 2.1, 
		   true, true, true);

  pMaker->BookPlot("jet1_pt", true,
		   "P_{T} of leading jet", "Number of Events / GeV",
		   0, 1500, 2.e-4, 8.e3,
		   0., 2.1, 
		   true, true, true);

  pMaker->BookPlot("jet2_pt", true,
		   "P_{T} of sub-leading jet", "Number of Events / GeV",
		   0, 1200, 2.e-4, 8.e3,
		   0., 2.1, 
		   true, true, true);

  pMaker->BookPlot("jet3_pt", true,
		   "P_{T} of third-leading jet", "Number of Events / GeV",
		   0, 800, 2.e-4, 8.e3,
		   0., 2.1, 
		   true, true, true);

  pMaker->BookPlot("btag1_pt", true,
		   "P_{T} of leading btag", "Number of Events / GeV",
		   0, 1400, 2.e-4, 8.e3,
		   0., 2.1, 
		   true, true, true);

  pMaker->BookPlot("w_mT_t01", true,
		   "W Transverse Mass", "Number of Events / GeV",
		   0, 1000, 2.e-4, 8.e3,
		   0., 2.1, 
		   true, true, true);

  pMaker->BookPlot("dPhi_met_l", false,
		   "#Delta#phi(l, #slash{E}_T)", "Number of Events",
		   -3.2, 3.2, 2.e-4, 8.e3,
		   0., 2.1,
		   true, false, false);

  pMaker->BookPlot("dPhi_met_ht", false,
		   "#Delta#phi(l, #vec{HT})", "Number of Events",
		   -3.2, 3.2, 2.e-4, 8.e3,
		   0., 2.1,
		   true, false, false);

  if(channels[channel].Contains("ele")) {
    pMaker->BookPlot("ele_pt", true,
		     "Electron P_{T}", "Number of Events / GeV",
		     0, 1500, 2.e-4, 8.e3,
		     0., 2.1, 
		     true, true, true);

    pMaker->BookPlot("ele_eta", false,
		     "Electron #eta", "Number of Events / GeV",
		     -2.4, 2.4, 2.e-4, 8.e3,
		     0., 2.1, 
		     true, false, false);
  }
  else if(channels[channel].Contains("muon")) {
    pMaker->BookPlot("muon_pt", true,
		     "Muon P_{T}", "Number of Events / GeV",
		     0, 2000, 2.e-4, 8.e3,
		     0., 2.1, 
		     true, true, true);

    pMaker->BookPlot("muon_eta", false,
		     "Muon #eta", "Number of Events / GeV",
		     -2.4, 2.4, 2.e-4, 8.e3,
		     0., 2.1, 
		     true, false, false);
  }

  if(controlRegion != kCR0 && controlRegion != kAny) {
    pMaker->BookPlot("leadSigmaIetaIeta", false,
		     "lead #sigma_{i#eta i#eta}", "Number of Events",
		     0, 0.035, 2.e-5, 8.e3,
		     0, 2.1,
		     true, true, true);

    pMaker->BookPlot("leadPhotonEta", false,
		     "#eta of leading #gamma", "Number of Events",
		     -1.5, 1.5, 2.e-3, 3.e4,
		     0., 2.1,
		     false, false, false);

    pMaker->BookPlot("leadPhotonPhi", false,
		     "#phi of leading #gamma", "Number of Events",
		     -3.2, 3.2, 2.e-3, 3.e4,
		     0., 2.1,
		     false, false, false);

    if(controlRegion == kCR1) {
      pMaker->BookPlot("leadPhotonEt", true,
		       "E_{T} of leading #gamma", "Number of Events / GeV",
		       0., 200., 2.e-4, 2.e4,
		       0., 1.9,
		       false, true, true);
    }
    else if(controlRegion == kCR2) {
      pMaker->BookPlot("leadPhotonEt", true,
		       "E_{T} of leading #gamma", "Number of Events / GeV",
		       0., 199.9, 2.e-4, 40.0,
		       0., 1.9,
		       false, true, true);
    }
    else {
      pMaker->BookPlot("leadPhotonEt", true,
		       "E_{T} of leading #gamma", "Number of Events / GeV",
		       0., 700., 2.e-4, 5.e2,
		       0., 5.1,
		       true, true, true);
    }

    pMaker->BookPlot("mLepGammaLead", true,
		     "m_{l#gamma_{lead}}", "Number of Events / GeV",
		     0., 400., 2.e-3, 5.e4,
		     0., 1.9, 
		     true, true, true);
  }

  if(controlRegion == kSR2 || controlRegion == kCR2 || controlRegion == kCR2a) {
    pMaker->BookPlot("trailSigmaIetaIeta", false,
		     "trail #sigma_{i#eta i#eta}", "Number of Events",
		     0, 0.035, 2.e-5, 8.e3,
		     0, 2.1,
		     true, true, true);

    pMaker->BookPlot("trailPhotonEta", false,
		     "#eta of trailing #gamma", "Number of Events",
		     -1.5, 1.5, 2.e-3, 3.e4,
		     0., 2.1,
		     false, false, false);

    pMaker->BookPlot("trailPhotonPhi", false,
		     "#phi of trailing #gamma", "Number of Events",
		     -3.2, 3.2, 2.e-3, 3.e4,
		     0., 2.1,
		     false, false, false);

    pMaker->BookPlot("trailPhotonEt", true,
		     "E_{T} of trailing #gamma", "Number of Events / GeV",
		     0., 700., 2.e-4, 5.e2,
		     0., 5.1,
		     true, true, true);

    pMaker->BookPlot("mLepGammaTrail", true,
		     "m_{l#gamma_{trail}}", "Number of Events / GeV",
		     0., 1200., 2.e-3, 5.e4,
		     0., 5.1, 
		     true, true, true);

    pMaker->BookPlot("photon_dR", false,
		     "#DeltaR_{#gamma#gamma}", "Number of Events",
		     0.5, 5., 2.e-2, 3.e5,
		     0., 2.1,
		     true, false, false);

    pMaker->BookPlot("photon_dPhi", false,
		     "#Delta#phi_{#gamma#gamma}", "Number of Events",
		     0., 3.14159, 2.e-2, 3.e5,
		     0., 2.1,
		     true, false, false);

    pMaker->BookPlot("photon_invmass", true,
		     "m_{#gamma#gamma} (GeV/c^{2})", "Number of Events",
		     0, 2000, 2.e-3, 3.e4,
		     0., 11.5,
		     true, true, true);

    pMaker->BookPlot("diEMpT", true,
		     "di-EM Pt", "Number of Events",
		     0, 1200, 2.e-3, 5.e4,
		     0., 5.1,
		     true, true, true);
    
    pMaker->BookPlot("diJetPt", true,
		     "di-Jet Pt", "Number of Events",
		     0, 1400, 2.e-3, 5.e4,
		     0., 5.1,
		     true, true, true);

    pMaker->BookPlot("mLepGammaGamma", true,
		     "m_{l#gamma#gamma}", "Number of Events",
		     0, 1200, 2.e-3, 5.e4,
		     0., 5.1,
		     true, true, true);
  }

  pMaker->CreatePlots();

  delete pMaker;

}
