from utils import *
from array import array

def normalizeQCD(input, channel, systematic, wjetsResults, topM3Results, dilepResults, eleFakeRateResults):

    (wjetsSF, wjetsSFerror) = wjetsResults
    (topM3sf, topM3sfError) = topM3Results
    (dilepSF, dilepSFerror) = dilepResults
    (eleFakeRateSF, eleFakeRateSFerror) = eleFakeRateResults

    varName = 'pfMET_t01'

    qcdName = '_qcd_'
    systName = systematic

    if systematic == '_qcdDefUp':
        qcdName = '_qcd_relIso_10_'
        systName = ''
    elif systematic == '_qcdDefDown':
        qcdName = '_qcd_relIso_m10_'
        systName = ''

    dataHist = get1DHist(input, varName+'_gg_'+channel)
    qcdHist = get1DHist(input, varName+qcdName+channel)

    topHist = get1DHist(input, varName+'_ttJetsHadronic_'+channel+systName)
    topHist.Add(get1DHist(input, varName+'_ttJetsSemiLep_'+channel+systName))
    topHist.Add(get1DHist(input, varName+'_ttJetsFullLep_'+channel+systName))
    ScaleWithError(topHist, topM3sf, topM3sfError)

    wjetsHist = get1DHist(input, varName+'_W3JetsToLNu_'+channel+systName)
    wjetsHist.Add(get1DHist(input, varName+'_W4JetsToLNu_'+channel+systName))
    ScaleWithError(wjetsHist, wjetsSF, wjetsSFerror)

    zHist = get1DHist(input, varName+'_dy1JetsToLL_'+channel+systName)
    zHist.Add(get1DHist(input, varName+'_dy2JetsToLL_'+channel+systName))
    zHist.Add(get1DHist(input, varName+'_dy3JetsToLL_'+channel+systName))
    zHist.Add(get1DHist(input, varName+'_dy4JetsToLL_'+channel+systName))
    zHist.Add(get1DHist(input, varName+'_ZGToLLG_'+channel+systName))
    zHist.Add(get1DHist(input, varName+'_WGToLNuG_'+channel+systName))
    ScaleWithError(zHist, dilepSF, dilepSFerror)
    ScaleWithError(zHist, eleFakeRateSF, eleFakeRateSFerror)

    MCHist = get1DHist(input, varName+'_TBar_s_'+channel+systName)
    MCHist.Add(get1DHist(input, varName+'_TBar_t_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_TBar_tW_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_T_s_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_T_t_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_T_tW_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_TTGamma_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_WW_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_WZ_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_ZZ_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_TTWJets_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_TTZJets_'+channel+systName))

    MCHist.Add(topHist)
    MCHist.Add(wjetsHist)
    MCHist.Add(zHist)

    lowbin = 1
    highbin = dataHist.FindBin(20.0) - 1

    ndata = dataHist.Integral(lowbin, highbin)
    nqcd = qcdHist.Integral(lowbin, highbin)
    nmc = MCHist.Integral(lowbin, highbin)

    sigma_data = 0
    sigma_qcd = 0
    sigma_mc = 0

    for ibin in range(highbin):
        sigma_data += pow(dataHist.GetBinError(ibin+1), 2)
        sigma_qcd += pow(qcdHist.GetBinError(ibin+1), 2)
        sigma_mc += pow(MCHist.GetBinError(ibin+1), 2)

    sigma_data = math.sqrt(sigma_data)
    sigma_qcd = math.sqrt(sigma_qcd)
    sigma_mc = math.sqrt(sigma_mc)

    qcdScale = (ndata - nmc) / nqcd;
    if qcdScale < 0:
        return (0.0, 0.0)

    qcdScaleError = (pow(sigma_data, 2) + pow(sigma_mc, 2)) / pow(ndata - nmc, 2)
    qcdScaleError += pow(sigma_qcd, 2) / pow(nqcd, 2)
    qcdScaleError = qcdScale * math.sqrt(qcdScaleError)

    return (qcdScale, qcdScaleError)

def doQCDFit(channel, controlRegion, systematic, output, output_allMC_qcd, xlo, xhi, dilepResults, axisMin, axisMax, doLogy):

    (dilepSF, dilepSFerror) = dilepResults

    input = '../histograms_'+channel+'_'+controlRegion+'.root'

    varName = 'pfMET_t01'

    qcdName = '_qcd_'
    systName = systematic

    if systematic == '_qcdDefUp':
        qcdName = '_qcd_relIso_10_'
        systName = ''
    elif systematic == '_qcdDefDown':
        qcdName = '_qcd_relIso_m10_'
        systName = ''

    dataHist = get1DHist(input, varName+'_gg_'+channel)
    qcdHist = get1DHist(input, varName+qcdName+channel)

    MCHist = get1DHist(input, varName+'_dy1JetsToLL_'+channel+systName)
    MCHist.Add(get1DHist(input, varName+'_dy2JetsToLL_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_dy3JetsToLL_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_dy4JetsToLL_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_ZGToLLG_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_WGToLNuG_'+channel+systName))
    ScaleWithError(MCHist, dilepSF, dilepSFerror)

    MCHist.Add(get1DHist(input, varName+'_ttJetsHadronic_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_ttJetsSemiLep_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_ttJetsFullLep_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_W3JetsToLNu_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_W4JetsToLNu_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_TBar_s_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_TBar_t_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_TBar_tW_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_T_s_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_T_t_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_T_tW_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_TTGamma_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_WW_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_WZ_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_ZZ_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_TTWJets_'+channel+systName))
    MCHist.Add(get1DHist(input, varName+'_TTZJets_'+channel+systName))

    (dataInt, dataIntError) = integrateError(dataHist, xlo, xhi)
    (qcdInt, qcdIntError) = integrateError(qcdHist, xlo, xhi)
    (bkgInt, bkgIntError) = integrateError(MCHist, xlo, xhi)

    (qcdFrac, qcdFracErr) = makeFit(varName+'', xlo, xhi, qcdHist, MCHist, dataHist)

    QCDSF = qcdFrac * dataInt / qcdInt
    QCDSFerror = QCDSF * ( (qcdFracErr/qcdFrac)**2 + (dataIntError/dataInt)**2 + (qcdIntError/qcdInt)**2 )**0.5

    bkgSF = (1.0-qcdFrac) * dataInt / bkgInt
    bkgSFerror = bkgSF * ( (qcdFracErr/(1.0-qcdFrac))**2 + (dataIntError/dataInt)**2 + (bkgIntError/bkgInt)**2 )**0.5

    if systematic == '':
        output.write('central\t'+
                     str(QCDSF)+'\t'+
                     str(QCDSFerror)+'\n')
        output_allMC_qcd.write('central\t'+
                               str(bkgSF)+'\t'+
                               str(bkgSFerror)+'\n')

        #drawPlots(dataHist, qcdHist, QCDSF, 'QCD', MCHist, 1.0, 'MC', xlo, xhi, varName+'_'+channel+systematic, '#slash{E}_{T} (GeV)', axisMin, axisMax, doLogy)
        drawPlots(dataHist, qcdHist, QCDSF, 'QCD', MCHist, bkgSF, 'MC', xlo, xhi, varName+'_'+channel+systematic, '#slash{E}_{T} (GeV)', axisMin, axisMax, doLogy)
    else:
        output.write(systematic+'\t'+
                     str(QCDSF)+'\t'+
                     str(QCDSFerror)+'\n')
        output_allMC_qcd.write(systematic+'\t'+
                               str(bkgSF)+'\t'+
                               str(bkgSFerror)+'\n')

    return (QCDSF, QCDSFerror)

def doM3Fit(channel, controlRegion, systematic, output_wjets, output_ttbar, xlo, xhi, qcdSFPair, dilepPair, axisMin, axisMax, doLogy):

    (QCDSF, QCDSFerror) = qcdSFPair
    (dilepSF, dilepSFerror) = dilepPair

    input = '../histograms_'+channel+'_'+controlRegion+'.root'

    qcdName = '_qcd_'
    systName = systematic

    if systematic == '_qcdDefUp':
        qcdName = '_qcd_relIso_10_'
        systName = ''
    elif systematic == '_qcdDefDown':
        qcdName = '_qcd_relIso_m10_'
        systName = ''

    dataHist = get1DHist(input, 'm3_gg_'+channel)

    topHist = get1DHist(input, 'm3_ttJetsHadronic_'+channel+systName)
    topHist.Add(get1DHist(input, 'm3_ttJetsFullLep_'+channel+systName))
    topHist.Add(get1DHist(input, 'm3_ttJetsSemiLep_'+channel+systName))

    wjetsHist = get1DHist(input, 'm3_W3JetsToLNu_'+channel+systName)
    wjetsHist.Add(get1DHist(input, 'm3_W4JetsToLNu_'+channel+systName))

    qcdHist = get1DHist(input, 'm3'+qcdName+channel)
    ScaleWithError(qcdHist, QCDSF, QCDSFerror)

    bkgHist = get1DHist(input, 'm3_dy1JetsToLL_'+channel+systName)
    bkgHist.Add(get1DHist(input, 'm3_dy2JetsToLL_'+channel+systName))
    bkgHist.Add(get1DHist(input, 'm3_dy3JetsToLL_'+channel+systName))
    bkgHist.Add(get1DHist(input, 'm3_dy4JetsToLL_'+channel+systName))
    bkgHist.Add(get1DHist(input, 'm3_ZGToLLG_'+channel+systName))
    bkgHist.Add(get1DHist(input, 'm3_WGToLNuG_'+channel+systName))
    ScaleWithError(bkgHist, dilepSF, dilepSFerror)

    bkgHist.Add(get1DHist(input, 'm3_TBar_s_'+channel+systName))
    bkgHist.Add(get1DHist(input, 'm3_TBar_t_'+channel+systName))
    bkgHist.Add(get1DHist(input, 'm3_TBar_tW_'+channel+systName))
    bkgHist.Add(get1DHist(input, 'm3_T_s_'+channel+systName))
    bkgHist.Add(get1DHist(input, 'm3_T_t_'+channel+systName))
    bkgHist.Add(get1DHist(input, 'm3_T_tW_'+channel+systName))
    bkgHist.Add(get1DHist(input, 'm3_WW_'+channel+systName))
    bkgHist.Add(get1DHist(input, 'm3_WZ_'+channel+systName))
    bkgHist.Add(get1DHist(input, 'm3_ZZ_'+channel+systName))
    bkgHist.Add(get1DHist(input, 'm3_TTWJets_'+channel+systName))
    bkgHist.Add(get1DHist(input, 'm3_TTZJets_'+channel+systName))
    bkgHist.Add(get1DHist(input, 'm3_TTGamma_'+channel+systName))

    dataHist.Add(qcdHist, -1.0)
    dataHist.Add(bkgHist, -1.0)

    (dataInt, dataIntError) = integrateError(dataHist, xlo, xhi)
    (topInt, topIntError) = integrateError(topHist, xlo, xhi)
    (wjetsInt, wjetsIntError) = integrateError(wjetsHist, xlo, xhi)

    (fitFrac, fitFracErr) = makeFit('m3', xlo, xhi, topHist, wjetsHist, dataHist)

    topSF = fitFrac * dataInt / topInt
    topSFerror = topSF * ( (fitFracErr/fitFrac)**2 + (dataIntError/dataInt)**2 + (topIntError/topInt)**2 )**0.5

    wjetsSF = (1.0-fitFrac) * dataInt / wjetsInt
    wjetsSFerror = wjetsSF * ( (fitFracErr/(1.0-fitFrac))**2 + (dataIntError/dataInt)**2 + (wjetsIntError/wjetsInt)**2 )**0.5

    if systematic == '':
        output_wjets.write('central\t'+
                           str(wjetsSF)+'\t'+
                           str(wjetsSFerror)+'\n')

        output_ttbar.write('central\t'+
                           str(topSF)+'\t'+
                           str(topSFerror)+'\n')

        drawPlots(dataHist, topHist, topSF, 't#bar{t} + Jets', wjetsHist, wjetsSF, 'W + Jets', xlo, xhi, 'm3_'+channel+systematic, 'M3 (GeV/c^2)', axisMin, axisMax, doLogy)

    else:
        output_wjets.write(systematic+'\t'+
                           str(wjetsSF)+'\t'+
                           str(wjetsSFerror)+'\n')

        output_ttbar.write(systematic+'\t'+
                           str(topSF)+'\t'+
                           str(topSFerror)+'\n')

    return (topSF, topSFerror, wjetsSF, wjetsSFerror)

def doSigmaFit(varName, channel, controlRegion, systematic, output_ttbar, output_ttgamma, output_purity, xlo, xhi, wjetsResults, topM3Results, dilepResults, eleFakeRateResults, axisMin, axisMax, doLogy):

    (wjetsSF, wjetsSFerror) = wjetsResults
    (topM3sf, topM3sfError) = topM3Results
    (dilepSF, dilepSFerror) = dilepResults
    (eleFakeRateSF, eleFakeRateSFerror) = eleFakeRateResults

    input = '../histograms_'+channel+'_'+controlRegion+'.root'

    qcdName = '_qcd_'
    systName = systematic

    if systematic == '_qcdDefUp':
        qcdName = '_qcd_relIso_10_'
        systName = ''
    elif systematic == '_qcdDefDown':
        qcdName = '_qcd_relIso_m10_'
        systName = ''

    dataHist = get1DHist(input, varName+'_gg_'+channel)

    topHist = get1DHist(input, varName+'_ttJetsHadronic_'+channel+systName)
    topHist.Add(get1DHist(input, varName+'_ttJetsFullLep_'+channel+systName))
    topHist.Add(get1DHist(input, varName+'_ttJetsSemiLep_'+channel+systName))
    ScaleWithError(topHist, topM3sf, topM3sfError)

    ttgammaHist = get1DHist(input, varName+'_TTGamma_'+channel+systName)

    bkgHist = get1DHist(input, varName+'_TBar_s_'+channel+systName)
    bkgHist.Add(get1DHist(input, varName+'_TBar_t_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_TBar_tW_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_T_s_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_T_t_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_T_tW_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_WW_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_WZ_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_ZZ_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_TTWJets_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_TTZJets_'+channel+systName))

    wjetsHist = get1DHist(input, varName+'_W3JetsToLNu_'+channel+systName)
    wjetsHist.Add(get1DHist(input, varName+'_W4JetsToLNu_'+channel+systName))
    ScaleWithError(wjetsHist, wjetsSF, wjetsSFerror)

    zHist = get1DHist(input, varName+'_dy1JetsToLL_'+channel+systName)
    zHist.Add(get1DHist(input, varName+'_dy2JetsToLL_'+channel+systName))
    zHist.Add(get1DHist(input, varName+'_dy3JetsToLL_'+channel+systName))
    zHist.Add(get1DHist(input, varName+'_dy4JetsToLL_'+channel+systName))
    zHist.Add(get1DHist(input, varName+'_ZGToLLG_'+channel+systName))
    zHist.Add(get1DHist(input, varName+'_WGToLNuG_'+channel+systName))
    ScaleWithError(zHist, dilepSF, dilepSFerror)
    ScaleWithError(zHist, eleFakeRateSF, eleFakeRateSFerror)

    qcdHist = get1DHist(input, varName+qcdName+channel)
    (qcdSF, qcdSFerror) = normalizeQCD(input, channel, systematic, wjetsResults, topM3Results, dilepResults, eleFakeRateResults)
    ScaleWithError(qcdHist, qcdSF, qcdSFerror)

    dataHist.Add(bkgHist, -1.0)
    #dataHist.Add(qcdHist, -1.0)
    dataHist.Add(wjetsHist, -1.0)
    dataHist.Add(zHist, -1.0)

    (dataInt, dataIntError) = integrateError(dataHist, xlo, xhi)
    (topInt, topIntError) = integrateError(topHist, xlo, xhi)
    (ttgammaInt, ttgammaIntError) = integrateError(ttgammaHist, xlo, xhi)

    regularCut = 0.012
    if varName == 'leadChargedHadronIso':
        regularCut = 2.6
    (dataInt_passCut, dataIntError_passCut) = integrateError(dataHist, 0, regularCut)
    (topInt_passCut, topIntError_passCut) = integrateError(topHist, 0, regularCut)
    (ttgammaInt_passCut, ttgammaIntError_passCut) = integrateError(ttgammaHist, 0, regularCut)

    (fitFrac, fitFracErr) = makeFit(varName, xlo, xhi, ttgammaHist, topHist, dataHist)

    ttgammaSF = fitFrac * dataInt / ttgammaInt
    ttgammaSFerror = ttgammaSF * ( (fitFracErr/fitFrac)**2 + (dataIntError/dataInt)**2 + (ttgammaIntError/ttgammaInt)**2 )**0.5

    topSF = (1.0-fitFrac) * dataInt / topInt
    topSFerror = topSF * ( (fitFracErr/(1.0-fitFrac))**2 + (dataIntError/dataInt)**2 + (topIntError/topInt)**2 )**0.5

    purity = ttgammaSF * ttgammaInt_passCut / (ttgammaSF * ttgammaInt_passCut + topSF * topInt_passCut)
    purityError = ttgammaSFerror*ttgammaSFerror/ttgammaSF/ttgammaSF + topSFerror*topSFerror/topSF/topSF + ttgammaIntError_passCut*ttgammaIntError_passCut/ttgammaInt_passCut/ttgammaInt_passCut + topIntError_passCut*topIntError_passCut/topInt_passCut/topInt_passCut
    purityError = math.sqrt(purityError) * topSF * topInt_passCut
    purityError = purityError / (ttgammaSF * ttgammaInt_passCut + topSF * topInt_passCut)

    if systematic == '':
        output_ttbar.write('central\t'+
                           str(topSF)+'\t'+
                           str(topSFerror)+'\n')

        output_ttgamma.write('central\t'+
                             str(ttgammaSF)+'\t'+
                             str(ttgammaSFerror)+'\n')

        output_purity.write('central\t'+
                            str(purity)+'\t'+
                            str(purityError)+'\n')

        xaxisLabel = '#sigma_{i#eta i#eta}'
        if varName == 'leadChargedHadronIso':
            xaxisLabel = 'Ch. Hadron Iso. (GeV)'

        drawPlots(dataHist, ttgammaHist, ttgammaSF, 't#bar{t} + #gamma', topHist, topSF, 't#bar{t} + Jets', xlo, xhi, varName+'_'+channel+systematic, xaxisLabel, axisMin, axisMax, doLogy)
    else:
        output_ttbar.write(systematic+'\t'+
                           str(topSF)+'\t'+
                           str(topSFerror)+'\n')

        output_ttgamma.write(systematic+'\t'+
                             str(ttgammaSF)+'\t'+
                             str(ttgammaSFerror)+'\n')

        output_purity.write(systematic+'\t'+
                            str(1.0-fitFrac)+'\t'+
                            str(fitFracErr)+'\n')

    return (topSF, topSFerror, ttgammaSF, ttgammaSFerror)

def doElectronFit(channel, controlRegion, systematic, output_z, xlo, xhi, dilepResults, axisMin, axisMax, doLogy):

    (dilepSF, dilepSFerror) = dilepResults

    varName = 'mLepGammaLead'

    inputMatched = '../fitTemplates.root'
    input = '../histograms_'+channel+'_'+controlRegion+'.root'

    qcdName = '_qcd_'
    systName = systematic

    if systematic == '_qcdDefUp':
        qcdName = '_qcd_relIso_10_'
        systName = ''
    elif systematic == '_qcdDefDown':
        qcdName = '_qcd_relIso_m10_'
        systName = ''

    dataHist = get1DHist(input, varName+'_gg_'+channel)

    zHist = get1DHist(inputMatched, varName+'_dy1JetsToLL_'+channel+'_'+controlRegion+'_matchElectron'+systName)
    zHist.Add(get1DHist(inputMatched, varName+'_dy2JetsToLL_'+channel+'_'+controlRegion+'_matchElectron'+systName))
    zHist.Add(get1DHist(inputMatched, varName+'_dy3JetsToLL_'+channel+'_'+controlRegion+'_matchElectron'+systName))
    zHist.Add(get1DHist(inputMatched, varName+'_dy4JetsToLL_'+channel+'_'+controlRegion+'_matchElectron'+systName))
    zHist.Add(get1DHist(inputMatched, varName+'_ZGToLLG_'+channel+'_'+controlRegion+'_matchElectron'+systName))
    zHist.Add(get1DHist(inputMatched, varName+'_WGToLNuG_'+channel+'_'+controlRegion+'_matchElectron'+systName))
    ScaleWithError(zHist, dilepSF, dilepSFerror)

    bkgHist = get1DHist(inputMatched, varName+'_dy1JetsToLL_'+channel+'_'+controlRegion+'_matchPhoton'+systName)
    bkgHist.Add(get1DHist(inputMatched, varName+'_dy2JetsToLL_'+channel+'_'+controlRegion+'_matchPhoton'+systName))
    bkgHist.Add(get1DHist(inputMatched, varName+'_dy3JetsToLL_'+channel+'_'+controlRegion+'_matchPhoton'+systName))
    bkgHist.Add(get1DHist(inputMatched, varName+'_dy4JetsToLL_'+channel+'_'+controlRegion+'_matchPhoton'+systName))
    bkgHist.Add(get1DHist(inputMatched, varName+'_ZGToLLG_'+channel+'_'+controlRegion+'_matchPhoton'+systName))
    bkgHist.Add(get1DHist(inputMatched, varName+'_WGToLNuG_'+channel+'_'+controlRegion+'_matchPhoton'+systName))

    bkgHist.Add(get1DHist(inputMatched, varName+'_dy1JetsToLL_'+channel+'_'+controlRegion+'_matchJet'+systName))
    bkgHist.Add(get1DHist(inputMatched, varName+'_dy2JetsToLL_'+channel+'_'+controlRegion+'_matchJet'+systName))
    bkgHist.Add(get1DHist(inputMatched, varName+'_dy3JetsToLL_'+channel+'_'+controlRegion+'_matchJet'+systName))
    bkgHist.Add(get1DHist(inputMatched, varName+'_dy4JetsToLL_'+channel+'_'+controlRegion+'_matchJet'+systName))
    bkgHist.Add(get1DHist(inputMatched, varName+'_ZGToLLG_'+channel+'_'+controlRegion+'_matchJet'+systName))
    bkgHist.Add(get1DHist(inputMatched, varName+'_WGToLNuG_'+channel+'_'+controlRegion+'_matchJet'+systName))

    ScaleWithError(bkgHist, dilepSF, dilepSFerror)

    bkgHist.Add(get1DHist(input, varName+'_TBar_s_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_TBar_t_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_TBar_tW_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_T_s_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_T_t_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_T_tW_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_WW_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_WZ_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_ZZ_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_TTWJets_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_TTZJets_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_ttJetsHadronic_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_ttJetsFullLep_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_ttJetsSemiLep_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_TTGamma_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_W3JetsToLNu_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_W4JetsToLNu_'+channel+systName))

    qcdHist = get1DHist(input, varName+qcdName+channel)

    dummyResults = (1.0, 0.0)

    (qcdSF, qcdSFerror) = normalizeQCD(input, channel, systematic, dummyResults, dummyResults, dilepResults, dummyResults)
    ScaleWithError(qcdHist, qcdSF, qcdSFerror)

    bkgHist.Add(qcdHist)

    (dataInt, dataIntError) = integrateError(dataHist, xlo, xhi)
    (zInt, zIntError) = integrateError(zHist, xlo, xhi)
    (bkgInt, bkgIntError) = integrateError(bkgHist, xlo, xhi)

    (fitFrac, fitFracErr) = makeFit(varName, xlo, xhi, zHist, bkgHist, dataHist)

    zSF = fitFrac * dataInt / zInt
    zSFerror = zSF * ( (fitFracErr/fitFrac)**2 + (dataIntError/dataInt)**2 + (zIntError/zInt)**2 )**0.5

    bkgSF = (1.0-fitFrac) * dataInt / bkgInt
    bkgSFerror = bkgSF * ( (fitFracErr/(1.0-fitFrac))**2 + (dataIntError/dataInt)**2 + (bkgIntError/bkgInt)**2 )**0.5

    if systematic == '':
        output_z.write('central\t'+
                       str(zSF)+'\t'+
                       str(zSFerror)+'\n')

        mcRate = zInt / (zInt+bkgInt)
        output_eleMisIDRate = open('eleMisIDRate_'+channel+'.txt', 'w')
        output_eleMisIDRate.write(str(mcRate)+'\t'+str(fitFrac)+'\n')
        output_eleMisIDRate.close()

        #drawPlots(dataHist, zHist, zSF, 'Z(#gamma) + Jets', bkgHist, bkgSF, 'Bkg', xlo, xhi, varName+'_'+channel+systematic, 'm_{l, #gamma} (GeV)', axisMin, axisMax, doLogy)
        drawPlots_tdrstyle(dataHist, zHist, zSF, 'Z(#gamma) + Jets', bkgHist, bkgSF, 'Bkg', xlo, xhi, varName+'_'+channel+systematic, 'm_{l, #gamma} (GeV)', axisMin, axisMax, doLogy)
    else:
        output_z.write(systematic+'\t'+
                       str(zSF)+'\t'+
                       str(zSFerror)+'\n')

    return (zSF, zSFerror)

def doDileptonFit(channel, controlRegion, systematic, output, xlo, xhi, axisMin, axisMax, doLogy):

    input = '../../zgamma/histograms_'+channel+'_'+controlRegion+'.root'

    varName = 'z_mass'

    dataHist = get1DHist(input, varName+'_gg_'+channel)

    systName = systematic
    if systematic == '_qcdDefUp' or systematic == '_qcdDefDown':
        systName = ''

    zHist = get1DHist(input, varName+'_dy1JetsToLL_'+channel+systName)
    zHist.Add(get1DHist(input, varName+'_dy2JetsToLL_'+channel+systName))
    zHist.Add(get1DHist(input, varName+'_dy3JetsToLL_'+channel+systName))
    zHist.Add(get1DHist(input, varName+'_dy4JetsToLL_'+channel+systName))
    zHist.Add(get1DHist(input, varName+'_ZGToLLG_'+channel+systName))
    zHist.Add(get1DHist(input, varName+'_WGToLNuG_'+channel+systName))

    bkgHist = get1DHist(input, varName+'_TBar_s_'+channel+systName)
    bkgHist.Add(get1DHist(input, varName+'_TBar_t_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_TBar_tW_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_T_s_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_T_t_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_T_tW_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_WW_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_WZ_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_ZZ_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_TTWJets_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_TTZJets_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_ttJetsHadronic_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_ttJetsFullLep_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_ttJetsSemiLep_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_TTGamma_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_W3JetsToLNu_'+channel+systName))
    bkgHist.Add(get1DHist(input, varName+'_W4JetsToLNu_'+channel+systName))

    (dataInt, dataIntError) = integrateError(dataHist, xlo, xhi)
    (zInt, zIntError) = integrateError(zHist, xlo, xhi)
    (bkgInt, bkgIntError) = integrateError(bkgHist, xlo, xhi)

    (fitFrac, fitFracErr) = makeFit(varName, xlo, xhi, zHist, bkgHist, dataHist)

    zSF = fitFrac * dataInt / zInt
    zSFerror = zSF * ( (fitFracErr/fitFrac)**2 + (dataIntError/dataInt)**2 + (zIntError/zInt)**2 )**0.5

    bkgSF = (1.0-fitFrac) * dataInt / bkgInt
    bkgSFerror = bkgSF * ( (fitFracErr/(1.0-fitFrac))**2 + (dataIntError/dataInt)**2 + (bkgIntError/bkgInt)**2 )**0.5

    if systematic == '':
        output.write('central\t'+
                     str(zSF)+'\t'+
                     str(zSFerror)+'\n')

        xaxisLabel = 'm_{ee} (GeV)'
        if channel == 'muon_bjj':
            xaxisLabel = 'm_{#mu#mu} (GeV)'

        #drawPlots(dataHist, zHist, zSF, 'Z(#gamma) + Jets', bkgHist, bkgSF, 'Bkg', xlo, xhi, varName+'_'+channel+systName, xaxisLabel, axisMin, axisMax, doLogy)
        drawPlots_tdrstyle(dataHist, zHist, zSF, 'Z(#gamma) + Jets', bkgHist, bkgSF, 'Bkg', xlo, xhi, varName+'_'+channel+systName, xaxisLabel, axisMin, axisMax, doLogy)

    else:
        output.write(systematic+'\t'+
                     str(zSF)+'\t'+
                     str(zSFerror)+'\n')

    return (zSF, zSFerror)

def doSigmaFitWithMatching(varName, channel, controlRegion, systematic, metCutName, output_jet, output_photon, output_purity, xlo, xhi, wjetsResults, topM3Results, dilepResults, eleFakeRateResults, axisMin, axisMax, doLogy):

    (wjetsSF, wjetsSFerror) = wjetsResults
    (topM3sf, topM3sfError) = topM3Results
    (dilepSF, dilepSFerror) = dilepResults
    (eleFakeRateSF, eleFakeRateSFerror) = eleFakeRateResults

    inputMatched = '../fitTemplates.root'

    qcdName = '_qcd_'
    systName = systematic

    if systematic == '_qcdDefUp':
        qcdName = '_qcd_relIso_10_'
        systName = ''
    elif systematic == '_qcdDefDown':
        qcdName = '_qcd_relIso_m10_'
        systName = ''

    extraWord = '_cutSigma'
    if varName == 'leadSigmaIetaIeta':
        extraWord = '_cutChHadIso'

    dataHist = get1DHist(inputMatched, varName+'_gg_'+channel+'_'+controlRegion+metCutName+extraWord)

    photonHist = get1DHist(inputMatched, varName+'_ttJetsHadronic_'+channel+'_'+controlRegion+metCutName+extraWord+'_matchPhoton'+systName)
    photonHist.Add(get1DHist(inputMatched, varName+'_ttJetsFullLep_'+channel+'_'+controlRegion+metCutName+extraWord+'_matchPhoton'+systName))
    photonHist.Add(get1DHist(inputMatched, varName+'_ttJetsSemiLep_'+channel+'_'+controlRegion+metCutName+extraWord+'_matchPhoton'+systName))
    photonHist.Add(get1DHist(inputMatched, varName+'_ttJetsHadronic_'+channel+'_'+controlRegion+metCutName+extraWord+'_matchElectron'+systName))
    photonHist.Add(get1DHist(inputMatched, varName+'_ttJetsFullLep_'+channel+'_'+controlRegion+metCutName+extraWord+'_matchElectron'+systName))
    photonHist.Add(get1DHist(inputMatched, varName+'_ttJetsSemiLep_'+channel+'_'+controlRegion+metCutName+extraWord+'_matchElectron'+systName))
    ScaleWithError(photonHist, topM3sf, topM3sfError)

    photonHist.Add(get1DHist(inputMatched, varName+'_TTGamma_'+channel+'_'+controlRegion+metCutName+extraWord+'_matchPhoton'+systName))
    photonHist.Add(get1DHist(inputMatched, varName+'_TTGamma_'+channel+'_'+controlRegion+metCutName+extraWord+'_matchElectron'+systName))

    jetHist = get1DHist(inputMatched, varName+'_ttJetsHadronic_'+channel+'_'+controlRegion+metCutName+extraWord+'_matchJet'+systName)
    jetHist.Add(get1DHist(inputMatched, varName+'_ttJetsFullLep_'+channel+'_'+controlRegion+metCutName+extraWord+'_matchJet'+systName))
    jetHist.Add(get1DHist(inputMatched, varName+'_ttJetsSemiLep_'+channel+'_'+controlRegion+metCutName+extraWord+'_matchJet'+systName))
    ScaleWithError(jetHist, topM3sf, topM3sfError)

    jetHist.Add(get1DHist(inputMatched, varName+'_TTGamma_'+channel+'_'+controlRegion+metCutName+extraWord+'_matchJet'+systName))

    bkgHist = get1DHist(inputMatched, varName+'_TBar_s_'+channel+'_'+controlRegion+metCutName+extraWord+systName)
    bkgHist.Add(get1DHist(inputMatched, varName+'_TBar_t_'+channel+'_'+controlRegion+metCutName+extraWord+systName))
    bkgHist.Add(get1DHist(inputMatched, varName+'_TBar_tW_'+channel+'_'+controlRegion+metCutName+extraWord+systName))
    bkgHist.Add(get1DHist(inputMatched, varName+'_T_s_'+channel+'_'+controlRegion+metCutName+extraWord+systName))
    bkgHist.Add(get1DHist(inputMatched, varName+'_T_t_'+channel+'_'+controlRegion+metCutName+extraWord+systName))
    bkgHist.Add(get1DHist(inputMatched, varName+'_T_tW_'+channel+'_'+controlRegion+metCutName+extraWord+systName))
    bkgHist.Add(get1DHist(inputMatched, varName+'_WW_'+channel+'_'+controlRegion+metCutName+extraWord+systName))
    bkgHist.Add(get1DHist(inputMatched, varName+'_WZ_'+channel+'_'+controlRegion+metCutName+extraWord+systName))
    bkgHist.Add(get1DHist(inputMatched, varName+'_ZZ_'+channel+'_'+controlRegion+metCutName+extraWord+systName))
    bkgHist.Add(get1DHist(inputMatched, varName+'_TTWJets_'+channel+'_'+controlRegion+metCutName+extraWord+systName))
    bkgHist.Add(get1DHist(inputMatched, varName+'_TTZJets_'+channel+'_'+controlRegion+metCutName+extraWord+systName))

    wjetsHist = get1DHist(inputMatched, varName+'_W3JetsToLNu_'+channel+'_'+controlRegion+metCutName+extraWord+systName)
    wjetsHist.Add(get1DHist(inputMatched, varName+'_W4JetsToLNu_'+channel+'_'+controlRegion+metCutName+extraWord+systName))
    ScaleWithError(wjetsHist, wjetsSF, wjetsSFerror)
    bkgHist.Add(wjetsHist)

    zHist = get1DHist(inputMatched, varName+'_dy1JetsToLL_'+channel+'_'+controlRegion+metCutName+extraWord+systName)
    zHist.Add(get1DHist(inputMatched, varName+'_dy2JetsToLL_'+channel+'_'+controlRegion+metCutName+extraWord+systName))
    zHist.Add(get1DHist(inputMatched, varName+'_dy3JetsToLL_'+channel+'_'+controlRegion+metCutName+extraWord+systName))
    zHist.Add(get1DHist(inputMatched, varName+'_dy4JetsToLL_'+channel+'_'+controlRegion+metCutName+extraWord+systName))
    zHist.Add(get1DHist(inputMatched, varName+'_ZGToLLG_'+channel+'_'+controlRegion+metCutName+extraWord+systName))
    zHist.Add(get1DHist(inputMatched, varName+'_WGToLNuG_'+channel+'_'+controlRegion+metCutName+extraWord+systName))
    ScaleWithError(zHist, dilepSF, dilepSFerror)
    ScaleWithError(zHist, eleFakeRateSF, eleFakeRateSFerror)
    bkgHist.Add(zHist)

    dataHist.Add(bkgHist, -1.0)

    (dataInt, dataIntError) = integrateError(dataHist, xlo, xhi)
    (jetInt, jetIntError) = integrateError(jetHist, xlo, xhi)
    (photonInt, photonIntError) = integrateError(photonHist, xlo, xhi)

    (fitFrac, fitFracErr) = makeFit(varName, xlo, xhi, photonHist, jetHist, dataHist)

    photonSF = fitFrac * dataInt / photonInt
    photonSFerror = photonSF * ( (fitFracErr/fitFrac)**2 + (dataIntError/dataInt)**2 + (photonIntError/photonInt)**2 )**0.5

    jetSF = (1.0-fitFrac) * dataInt / jetInt if (jetInt > 0.) else 0.
    jetSFerror = jetSF * ( (fitFracErr/(1.0-fitFrac))**2 + (dataIntError/dataInt)**2 + (jetIntError/jetInt)**2 )**0.5 if (jetInt > 0. and fitFrac < 1.) else 0.

    # purity measurement

    regularCut = 0.012
    if varName == 'leadChargedHadronIso':
        regularCut = 2.6

    jetHistScaled = jetHist.Clone('jetHistScaled_'+channel+'_'+controlRegion+systName+extraWord)
    ScaleWithError(jetHistScaled, jetSF, jetSFerror)

    photonHistScaled = photonHist.Clone('photonHistScaled_'+channel+'_'+controlRegion+systName+extraWord)
    ScaleWithError(photonHistScaled, photonSF, photonSFerror)

    (nJetMC, nJetMCErr) = integrateError(jetHist, xlo, regularCut)
    (nJetData, nJetDataErr) = integrateError(jetHistScaled, xlo, regularCut)

    (nPhotonMC, nPhotonMCErr) = integrateError(photonHist, xlo, regularCut)
    (nPhotonData, nPhotonDataErr) = integrateError(photonHistScaled, xlo, regularCut)

    purityMC = nPhotonMC / (nPhotonMC + nJetMC)
    purityMCError = math.sqrt(nPhotonMCErr*nPhotonMCErr/nPhotonMC/nPhotonMC + nJetMCErr*nJetMCErr/nJetMC/nJetMC)
    purityMCError = nPhotonMC*nJetMC*purityMCError/(nPhotonMC + nJetMC)/(nPhotonMC + nJetMC)

    purityData = nPhotonData / (nPhotonData + nJetData)
    purityDataError = math.sqrt(nPhotonDataErr*nPhotonDataErr/nPhotonData/nPhotonData + nJetDataErr*nJetDataErr/nJetData/nJetData)
    purityDataError = nPhotonData*nJetData*purityDataError/(nPhotonData + nJetData)/(nPhotonData + nJetData)

    if systematic == '':
        output_jet.write('central\t'+
                           str(jetSF)+'\t'+
                           str(jetSFerror)+'\n')

        output_photon.write('central\t'+
                             str(photonSF)+'\t'+
                             str(photonSFerror)+'\n')

        output_purity.write('central\t'+
                            str(purityMC)+'\t'+
                            str(purityMCError)+'\t'+
                            str(purityData)+'\t'+
                            str(purityDataError)+'\n')

        xaxisLabel = '#sigma_{i#eta i#eta}'
        if varName == 'leadChargedHadronIso':
            xaxisLabel = 'Ch. Hadron Iso. (GeV)'

        if varName == 'leadSigmaIetaIeta':
            dataHist.Rebin(2)
            photonHist.Rebin(2)
            jetHist.Rebin(2)
        if varName == 'leadChargedHadronIso':
            dataHist.Rebin(5)
            photonHist.Rebin(5)
            jetHist.Rebin(5)

        drawPlots(dataHist, photonHist, photonSF, 'Prompt t#bar{t}#gamma', jetHist, jetSF, 'Background', xlo, xhi, varName+'_'+channel+'_'+controlRegion+systematic, xaxisLabel, axisMin, axisMax, doLogy)
    else:
        output_jet.write(systematic+'\t'+
                           str(jetSF)+'\t'+
                           str(jetSFerror)+'\n')

        output_photon.write(systematic+'\t'+
                             str(photonSF)+'\t'+
                             str(photonSFerror)+'\n')

        output_purity.write(systematic+'\t'+
                            str(purityMC)+'\t'+
                            str(purityMCError)+'\t'+
                            str(purityData)+'\t'+
                            str(purityDataError)+'\n')


    return (jetSF, jetSFerror, photonSF, photonSFerror)

def wigglePurity(varName, outName, channel, controlRegion, systematic, metCutName, wjetsResults, topM3Results, dilepResults, eleFakeRateResults, promptResults, nonpromptResults):

    (wjetsSF, wjetsSFerror) = wjetsResults
    (topM3sf, topM3sfError) = topM3Results
    (dilepSF, dilepSFerror) = dilepResults
    (eleFakeRateSF, eleFakeRateSFerror) = eleFakeRateResults

    (promptSF, promptSFerror) = promptResults
    (nonpromptSF, nonpromptSFerror) = nonpromptResults

    inputMatched = '../fitTemplates.root'

    outputFile = ROOT.TFile('wigglePurity_'+channel+'_'+controlRegion+metCutName+'_'+outName+'.root', 'UPDATE')

    systName = systematic

    if systematic == '_qcdDefUp':
        systName = ''
    elif systematic == '_qcdDefDown':
        systName = ''

    extraWord = '_cutSigma'
    if varName == 'leadSigmaIetaIeta':
        extraWord = '_cutChHadIso'

    photonHist = get1DHist(inputMatched, varName+'_ttJetsHadronic_'+channel+'_'+controlRegion+metCutName+extraWord+'_matchPhoton'+systName)
    photonHist.Add(get1DHist(inputMatched, varName+'_ttJetsFullLep_'+channel+'_'+controlRegion+metCutName+extraWord+'_matchPhoton'+systName))
    photonHist.Add(get1DHist(inputMatched, varName+'_ttJetsSemiLep_'+channel+'_'+controlRegion+metCutName+extraWord+'_matchPhoton'+systName))
    photonHist.Add(get1DHist(inputMatched, varName+'_ttJetsHadronic_'+channel+'_'+controlRegion+metCutName+extraWord+'_matchElectron'+systName))
    photonHist.Add(get1DHist(inputMatched, varName+'_ttJetsFullLep_'+channel+'_'+controlRegion+metCutName+extraWord+'_matchElectron'+systName))
    photonHist.Add(get1DHist(inputMatched, varName+'_ttJetsSemiLep_'+channel+'_'+controlRegion+metCutName+extraWord+'_matchElectron'+systName))
    ScaleWithError(photonHist, topM3sf, topM3sfError)

    photonHist.Add(get1DHist(inputMatched, varName+'_TTGamma_'+channel+'_'+controlRegion+metCutName+extraWord+'_matchPhoton'+systName))
    photonHist.Add(get1DHist(inputMatched, varName+'_TTGamma_'+channel+'_'+controlRegion+metCutName+extraWord+'_matchElectron'+systName))

    jetHist = get1DHist(inputMatched, varName+'_ttJetsHadronic_'+channel+'_'+controlRegion+metCutName+extraWord+'_matchJet'+systName)
    jetHist.Add(get1DHist(inputMatched, varName+'_ttJetsFullLep_'+channel+'_'+controlRegion+metCutName+extraWord+'_matchJet'+systName))
    jetHist.Add(get1DHist(inputMatched, varName+'_ttJetsSemiLep_'+channel+'_'+controlRegion+metCutName+extraWord+'_matchJet'+systName))
    ScaleWithError(jetHist, topM3sf, topM3sfError)

    jetHist.Add(get1DHist(inputMatched, varName+'_TTGamma_'+channel+'_'+controlRegion+metCutName+extraWord+'_matchJet'+systName))

    before = photonHist.Clone(varName+'_before_'+channel+'_'+controlRegion+systName)
    before.Add(jetHist)
    outputFile.cd()
    before.Write(varName+'_before_'+channel+'_'+controlRegion+systName)

    after = photonHist.Clone(varName+'_after_'+channel+'_'+controlRegion+systName)
    ScaleWithError(after, promptSF, promptSFerror)
    ScaleWithError(jetHist, nonpromptSF, nonpromptSFerror)
    after.Add(jetHist)
    outputFile.cd()
    after.Write(varName+'_after_'+channel+'_'+controlRegion+systName)
        
    outputFile.Close()

def fixLimitInputs(channel, controlRegion, systematic, version, metCutName, wjetsResults, topM3Results, dilepResults, eleFakeRateResults, promptResults, nonpromptResults):

    (wjetsSF, wjetsSFerror) = wjetsResults
    (topM3sf, topM3sfError) = topM3Results
    (dilepSF, dilepSFerror) = dilepResults
    (eleFakeRateSF, eleFakeRateSFerror) = eleFakeRateResults

    (promptSF, promptSFerror) = promptResults
    (nonpromptSF, nonpromptSFerror) = nonpromptResults

    inputMatched = '../fitTemplates.root'
    output = ROOT.TFile('limitInputsScaled_bjj_'+version+metCutName+'.root', 'UPDATE')

    outputFolder = 'ele_'+controlRegion if ('ele' in channel) else 'muon_'+controlRegion

    if not output.GetDirectory(outputFolder):
        output.mkdir(outputFolder)

    systName = systematic

    if systematic == '_qcdDefUp':
        systName = ''
    elif systematic == '_qcdDefDown':
        systName = ''

    names = ['ttjets', 'wjets', 'zjets', 'singleTop', 'diboson', 'vgamma', 'ttW', 'ttZ', 'ttgamma']

    namesFull = [['ttJetsSemiLep', 'ttJetsFullLep', 'ttJetsHadronic'],
                 ['W3JetsToLNu', 'W4JetsToLNu'],
                 ['dy1JetsToLL', 'dy2JetsToLL', 'dy3JetsToLL', 'dy4JetsToLL'],
                 ['TBar_s', 'TBar_t', 'TBar_tW', 'T_s', 'T_t', 'T_tW'],
                 ['WW', 'WZ', 'ZZ'],
                 ['WGToLNuG', 'ZGToLLG'],
                 ['TTWJets'],
                 ['TTZJets'],
                 ['TTGamma']]

    nBins = 10
    xbins = [0, 10, 20, 30, 40, 50, 75, 100, 150, 300, 800]

    for i in range(0, len(names)):

        if names[i] == 'ttjets' or names[i] == 'ttgamma':
            photonHist = get1DHist(inputMatched, 'pfMET_t01_'+namesFull[i][0]+'_'+channel+'_'+controlRegion+'_matchPhoton'+systName)
            photonHist.Add(get1DHist(inputMatched, 'pfMET_t01_'+namesFull[i][0]+'_'+channel+'_'+controlRegion+'_matchElectron'+systName))

            jetHist = get1DHist(inputMatched, 'pfMET_t01_'+namesFull[i][0]+'_'+channel+'_'+controlRegion+'_matchJet'+systName)
        
            for j in range (1, len(namesFull[i])):
                photonHist.Add(get1DHist(inputMatched, 'pfMET_t01_'+namesFull[i][j]+'_'+channel+'_'+controlRegion+'_matchPhoton'+systName))
                photonHist.Add(get1DHist(inputMatched, 'pfMET_t01_'+namesFull[i][j]+'_'+channel+'_'+controlRegion+'_matchElectron'+systName))
                jetHist.Add(get1DHist(inputMatched, 'pfMET_t01_'+namesFull[i][j]+'_'+channel+'_'+controlRegion+'_matchJet'+systName))

            if names[i] == 'ttjets':
                ScaleWithError(photonHist, topM3sf, topM3sfError)
                ScaleWithError(jetHist, topM3sf, topM3sfError)

            (before, beforeErr) = integrateErrorSum(photonHist, jetHist)

            simpleScaling = photonHist.Clone(names[i]+systName+'_simpleScaling')
            simpleScaling.Add(jetHist)

            noScaling = photonHist.Clone(names[i]+systName+'_noScaling')
            noScaling.Add(jetHist)

            ScaleWithError(photonHist, promptSF, promptSFerror)
            ScaleWithError(jetHist, nonpromptSF, nonpromptSFerror)

            if controlRegion == 'SR2':
                ScaleWithError(photonHist, promptSF, promptSFerror)
                ScaleWithError(jetHist, nonpromptSF, nonpromptSFerror)

            (after, afterErr) = integrateErrorSum(photonHist, jetHist)
            
            scale_this = after / before if (before > 0.) else 0.
            scaleError_this = scale_this * ( (afterErr/after)**2 + (beforeErr/before)**2 )**0.5 if (after > 0. and before > 0.) else 0.

            ScaleWithError(simpleScaling, scale_this, scaleError_this)
            simpleScalingRebinned = simpleScaling.Rebin(nBins, names[i]+systName+'_simpleScaling_reb', array('d', xbins))

            photonHist.Add(jetHist)
            photonRebinned = photonHist.Rebin(nBins, names[i]+systName+'_reb', array('d', xbins))

            noScalingRebinned = noScaling.Rebin(nBins, names[i]+systName+'_noScaling_reb', array('d', xbins))

            output.cd(outputFolder)
            photonRebinned.Write(names[i]+systematic)
            simpleScalingRebinned.Write(names[i]+'_simpleScaling'+systematic)
            noScalingRebinned.Write(names[i]+'_noScaling'+systematic)

            if controlRegion == 'SR1':
                output_simpleScaling = open('puritySF_'+names[i]+'_'+channel+'_'+version+metCutName+'.txt', 'a')
                firstLine = systematic
                if systematic == '':
                    firstLine = 'central'
                    output_simpleScaling.write('systematic\tSF\tError\n')

                output_simpleScaling.write(firstLine+'\t'+
                                           str(scale_this)+'\t'+
                                           str(scaleError_this)+'\n')

                output_simpleScaling.close()

        else:
            hist = get1DHist(inputMatched, 'pfMET_t01_'+namesFull[i][0]+'_'+channel+'_'+controlRegion+systName)

            for j in range (1, len(namesFull[i])):
                hist.Add(get1DHist(inputMatched, 'pfMET_t01_'+namesFull[i][j]+'_'+channel+'_'+controlRegion+systName))

            if names[i] == 'wjets':
                ScaleWithError(hist, wjetsSF, wjetsSFerror)

            if names[i] == 'zjets' or names[i] == 'vgamma':
                ScaleWithError(hist, dilepSF, dilepSFerror)
                ScaleWithError(hist, eleFakeRateSF, eleFakeRateSFerror)

            histReb = hist.Rebin(nBins, names[i]+systName+'_reb', array('d', xbins))

            output.cd(outputFolder)
            hist.Write(names[i]+systematic)
        
    output.Close()
