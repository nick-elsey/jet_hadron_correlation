#!/bin/csh

#  A Slightly ugly way to make it as simple as possible to run different analyses,
#  And to make it easier to change variables and keep the book keeping simple
#  Command Line Arugments:
#  [1]: the analysis: either jet or dijet
#  [2]: if set to 'default', uses original A_j parameterization ( or jet-hadron )
#  [2]: if not using default settings, sets whether to use particle-by-particle
#       efficiency corrections
#
#  -----Di_jet selection criteria-----
#  [3]: whether to require a trigger in your leading jet
#  [4]: software trigger E threshold
#  [5]: subleading jet minimum pt ( for jet hadron, set to zero )
#  [6]: leading jet minimum pt ( jet min pt for jet-hadron )
#  [7]: jet pt max ( global maximum, both dijet and jet )
#  [8]: jet radius for clustering algorithm
#  [9]: hard constituent pt cut
#  [10]: bins in Eta for correlation histograms
#  [11]: bins in phi for correlation histograms
#
#  Output names and locations are generated by the script, and correspond to the above variables

# first make sure program is updated and exists
 make bin/auau_correlation || exit

if ( $1 == '-h') then
echo 'parameters:'
echo 'for defaults use "(di)/jet default"'
echo '1: analysis type [dijet/jet] (default: dijet)'
echo '2: use tracking efficiency corrections [true/false] (default: false)'
echo '3: require trigger in leading jet [true/false] (default: true)'
echo '4: require software trigger above this value (default: 6.0)'
echo '5: subleading jet min pt (default: 10)'
echo '6: leading jet min pt (default: 20)'
echo '7: jet pt max (default: 100)'
echo '8: jet resolution parameter (default: 0.4)'
echo '9: hard constituent pt cut (default: 2.0)'
echo '10: bins in correlation histograms in eta (default: 22)'
echo '11: bins in correlation histograms in phi (default: 22)'
exit
endif

set ExecPath = `pwd`
set analysis = $1
set execute = './bin/auau_correlation'
set base = /nfs/rhi/STAR/Data/HaddedAuAu14Low/AuAu14Pico

if ( $# != "11" && !( $2 == 'default' ) ) then
	echo 'Error: illegal number of parameters'
	exit
endif

echo $analysis
if ( $analysis != 'dijet' && $analysis != 'jet' ) then
	echo 'Error: unknown analysis type'
	exit
endif

# Arguments
set useEfficiency = $2
set triggerCoincidence = $3
set softTrig = $4
set subLeadPtMin = $5
set leadPtMin = $6
set jetPtMax = $7
set jetRadius = $8
set constPtCut = $9
set binsEta = $10
set binsPhi = $11

if ( $2 == 'default' ) then
	set useEfficiency = 'false'
	set triggerCoincidence = 'true'
  set softTrig = 6.0
	if ( $analysis == 'dijet' ) then
		set subLeadPtMin = 10.0
		set leadPtMin = 20.0
		set jetPtMax = 100.0
	else if ( $analysis == 'jet' ) then
		set subLeadPtMin = 0.0
		set leadPtMin = 15.0
		set jetPtMax = 20.0
	endif
	set jetRadius = 0.4
  set constPtCut = 2.0
  set binsEta = 22
  set binsPhi = 22
endif

# Create the folder name for output
set outFile = ${analysis}
set outFile = ${outFile}_trigger_${triggerCoincidence}_softTrig_${softTrig}_eff_${useEfficiency}_lead_${leadPtMin}_sub_${subLeadPtMin}_max_${jetPtMax}_rad_${jetRadius}_hardpt_${constPtCut}_eta_${binsEta}_phi_${binsPhi}
# Make the directories since they may not exist...
if ( ! -d out/y14low/${analysis}/${outFile} ) then
mkdir -p out/y14low/${analysis}/${outFile}
mkdir -p out/y14low/${analysis}/${outFile}/correlations
mkdir -p out/y14low/${analysis}/${outFile}/tree
mkdir -p out/y14low/${analysis}/${outFile}/mixing
endif

if ( ! -d log/y14low/${analysis}/${outFile} ) then
mkdir -p log/y14low/${analysis}/${outFile}
endif


# Now Submit jobs for each data file
foreach input ( ${base}* )

# Create the output file base name
set OutBase = `basename $input | sed 's/.root//g'`

# Make the output names and path
set outLocation = "out/y14low/${analysis}/${outFile}/"
set outName = correlations/corr_${OutBase}.root
set outNameTree = tree/tree_${OutBase}.root

# Input files
set Files = ${input}

# Logfiles. Thanks cshell for this "elegant" syntax to split err and out
set LogFile     = log/y14low/${analysis}/${outFile}/${analysis}_${OutBase}.log
set ErrFile     = log/y14low/${analysis}/${outFile}/${analysis}_${OutBase}.err

echo "Logging output to " $LogFile
echo "Logging errors to " $ErrFile

set arg = "$analysis $useEfficiency $triggerCoincidence $softTrig $subLeadPtMin $leadPtMin $jetPtMax $jetRadius $constPtCut $binsEta $binsPhi $outLocation $outName $outNameTree $Files"

qsub -V -l mem=7GB -o $LogFile -e $ErrFile -N auauCorr -- ${ExecPath}/submit/qwrap.sh ${ExecPath} $execute $arg

end
