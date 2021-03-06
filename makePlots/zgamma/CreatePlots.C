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

void CreatePlots(int channel, int controlRegion, bool needsQCD, TString metType, bool useWhizard) {

  gROOT->Reset();
  gROOT->SetBatch(true);
  gROOT->SetStyle("Plain");
  gStyle->SetOptStat(0000);
  gStyle->SetOptTitle(0);

  vector<Float_t> sf_ttbar, sfError_ttbar;
  vector<Float_t> sf_wjets, sfError_wjets;
  vector<Float_t> sf_ttgamma, sfError_ttgamma;

  vector<Float_t> sf_vgamma, sfError_vgamma;
  readFitResults("../v3/scaleFactors/dilepSF_"+channels[channel]+".txt", sf_vgamma, sfError_vgamma);

  vector<Float_t> sf_qcd, sfError_qcd;
  
  PlotMaker * pMaker = new PlotMaker(channel, controlRegion, needsQCD, metType, sf_qcd, sfError_qcd);

  vector<TString> ttJets;
  ttJets.push_back("ttJetsSemiLep");
  ttJets.push_back("ttJetsFullLep");
  ttJets.push_back("ttJetsHadronic");
  pMaker->BookMCLayer(ttJets, kGray, "ttjets", "t#bar{t} + Jets", kGG, kTTbar, sf_ttbar, sfError_ttbar);

  vector<TString> wJets;
  //wJets.push_back("W1JetsToLNu");
  //wJets.push_back("W2JetsToLNu");
  wJets.push_back("W3JetsToLNu");
  wJets.push_back("W4JetsToLNu");
  pMaker->BookMCLayer(wJets, kOrange-3, "wjets", "W + Jets", kQQ, kV, sf_wjets, sfError_wjets);

  vector<TString> zJets;
  zJets.push_back("dy1JetsToLL");
  zJets.push_back("dy2JetsToLL");
  zJets.push_back("dy3JetsToLL");
  zJets.push_back("dy4JetsToLL");
  pMaker->BookMCLayer(zJets, kYellow, "zjets", "Z/#gamma* + Jets", kQQ, kV, sf_vgamma, sfError_vgamma);

  vector<TString> singleTop;
  singleTop.push_back("TBar_s");
  singleTop.push_back("TBar_t");
  singleTop.push_back("TBar_tW");
  singleTop.push_back("T_s");
  singleTop.push_back("T_t");
  singleTop.push_back("T_tW");
  pMaker->BookMCLayer(singleTop, kRed, "singleTop", "Single top", kQG, kTTbar);

  vector<TString> diboson;
  diboson.push_back("WW");
  diboson.push_back("WZ");
  diboson.push_back("ZZ");
  pMaker->BookMCLayer(diboson, kViolet-2, "diboson", "VV, V#gamma", kQQ, kVV);

  vector<TString> vgamma;
  vgamma.push_back("WGToLNuG");
  vgamma.push_back("ZGToLLG");
  pMaker->BookMCLayer(vgamma, kAzure-2, "vgamma", "VV, V#gamma", kQQ, kVV, sf_vgamma, sfError_vgamma);

  vector<TString>  ttW;
  ttW.push_back("TTWJets");
  pMaker->BookMCLayer(ttW, kCyan, "ttW", "t#bar{t} + V", kQQ, kTTbar);

  vector<TString>  ttZ;
  ttZ.push_back("TTZJets");
  pMaker->BookMCLayer(ttZ, kOrange-5, "ttZ", "t#bar{t} + V", kGG, kTTbar);

  vector<TString> ttgamma;
  if(useWhizard) ttgamma.push_back("ttA_2to5");
  else ttgamma.push_back("TTGamma");
  pMaker->BookMCLayer(ttgamma, 8, "ttgamma", "t#bar{t} + #gamma", kGG, kTTbar, sf_ttgamma, sfError_ttgamma);

  ///////////////////////////////////////////////////////

  // aps15
  if(controlRegion == kAny) {
    pMaker->BookPlot(metType, true,
		     "#slash{E}_{T} (GeV)", "Number of Events / GeV",
		     0., 300., 2.e-3, 5.e3,
		     0., 1.9,
		     true, true, true);
  }
  else if(controlRegion == kCR1) {
    pMaker->BookPlot(metType, true,
		     "#slash{E}_{T} (GeV)", "Number of Events / GeV",
		     0., 800., 2.e-3, 1.e2,
		     0., 1.9,
		     false, true, true);
  }
  else if(controlRegion == kCR2) {
    pMaker->BookPlot(metType, true,
		     "#slash{E}_{T} (GeV)", "Number of Events / GeV",
		     0., 799.9, 3.e-4, 40.0,
		     0., 1.9,
		     false, true, true);
  }
  else if(controlRegion == kSR1) {
    pMaker->BookPlot(metType, true,
		     "#slash{E}_{T} (GeV)", "Number of Events / GeV",
		     0., 799.9, 9.e-4, 1.e2,
		     0., 1.9,
		     true, true, true);
  }
  else if(controlRegion == kSR2) {
    pMaker->BookPlot(metType, true,
		     "#slash{E}_{T} (GeV)", "Number of Events / GeV",
		     0., 799.9, 3.e-4, 4.0,
		     0., 1.9,
		     true, true, true);
  }
  else {
    pMaker->BookPlot(metType, true,
		     "#slash{E}_{T} (GeV)", "Number of Events / GeV",
		     0., 300., 7.e-3, 2.5e5,
		     0., 1.9,
		     true, true, true);
  }
  
  //if(controlRegion == kCR1) pMaker->SetDoRebinMET(true);

  pMaker->BookPlot("z_mass", false,
		   channels[channel].Contains("ele") ? "m_{ee}" : "m_{#mu#mu}", "Number of Events",
		   0, 400, 2.e-1, 8.e4,
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

  if(channels[channel].Contains("ele")) {
    pMaker->BookPlot("ele1_pt", true,
		     "Lead Electron P_{T}", "Number of Events / GeV",
		     0, 1500, 2.e-4, 8.e3,
		     0., 2.1, 
		     true, true, true);

    pMaker->BookPlot("ele1_eta", false,
		     "Lead Electron #eta", "Number of Events / GeV",
		     -2.4, 2.4, 2.e-4, 8.e3,
		     0., 2.1, 
		     true, false, false);

    pMaker->BookPlot("ele2_pt", true,
		     "Trail Electron P_{T}", "Number of Events / GeV",
		     0, 1500, 2.e-4, 8.e3,
		     0., 2.1, 
		     true, true, true);

    pMaker->BookPlot("ele2_eta", false,
		     "Trail Electron #eta", "Number of Events / GeV",
		     -2.4, 2.4, 2.e-4, 8.e3,
		     0., 2.1, 
		     true, false, false);
  }
  else if(channels[channel].Contains("muon")) {
    pMaker->BookPlot("muon1_pt", true,
		     "Lead Muon P_{T}", "Number of Events / GeV",
		     0, 2000, 2.e-4, 8.e3,
		     0., 2.1, 
		     true, true, true);

    pMaker->BookPlot("muon1_eta", false,
		     "Lead Muon #eta", "Number of Events / GeV",
		     -2.4, 2.4, 2.e-4, 8.e3,
		     0., 2.1, 
		     true, false, false);

    pMaker->BookPlot("muon2_pt", true,
		     "Trail Muon P_{T}", "Number of Events / GeV",
		     0, 2000, 2.e-4, 8.e3,
		     0., 2.1, 
		     true, true, true);

    pMaker->BookPlot("muon2_eta", false,
		     "Trail Muon #eta", "Number of Events / GeV",
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

  }

  pMaker->CreatePlots();

  delete pMaker;

}
