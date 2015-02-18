from fit import *

controlRegion = 'CR0'

channels = ['ele_bjj', 'muon_bjj']
systematics = ['',
               '_btagWeightUp', '_btagWeightDown',
               '_puWeightUp', '_puWeightDown',
               '_scaleUp', '_scaleDown',
               '_pdfUp', '_pdfDown',
               '_topPtUp', '_topPtDown',
               '_JECUp', '_JECDown',
               '_leptonSFUp', '_leptonSFDown',
               '_photonSFUp', '_photonSFDown']

for channel in channels:
    output = open('fitResults_'+channel+'.txt', 'w')
    output.write('systematic\ttopSF\ttopSFerror\twjetsSF\twjetsSFerror\tQCDSF\tQCDSFerror\tMCSF\tMCSFerror\n')

    for systematic in systematics:
        doM3Fit(channel, controlRegion, systematic, output, 70.0, 500.0)

    output.close()
