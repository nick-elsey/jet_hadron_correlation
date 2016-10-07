#!/bin/csh

# Script for event mixing auau trees on the grid
# Command line arguments:
# [1]: input directory ( it should have a long string of analysis settings generated from grid_auau_corr.csh )
# [2]: event mixing data ( either .root file or list )
# [3]: Is the data MB or HT?
# [4]: total number of events to look through
# [5]: number of events to mix with each trigger
#
# Can set default settings by only giving
# [1]: input directory
# [2]: 'default'


# first make sure program is updated and exists
make bin/event_mixing || exit

set ExecPath = `pwd`
set inputDir = $1
set execute = './bin/event_mixing'
set base = ${inputDir}/tree/tree

if ( $# != "5" && !( $2 == 'default' ) ) then
echo 'Error: illegal number of parameters'
exit
endif


# define arguments
set mixEvents = $2
set dataType = $3
set nEvents = $4
set eventsPerTrigger = $5

# make a base directory for logging
set logBase = 'basename $inputDir'

#made the log directory
if ( ! -d log/mix/y14/${logBase} ) then
mkdir -p log/mix/y14/${logBase}
endif


if ( $2 == 'default' ) then
set mixEvents = 'auau_list/grid_AuAuy14MBHigh.list'
set dataType = 'MB'
set nEvents = '-1'
set eventsPerTrigger = '100'

# make the new output location
if ( ! -d ${inputDir}/mixing_y14 ) then
mkdir -p ${inputDir}/mixing_y14
endif

# Now Submit jobs for each data file
foreach input ( ${base}*.root )

# Create the output file base name
set OutBase = `basename $input | sed 's/.root//g'`

# Make the output names and path
set outName = mixing_y14/mixY14_${OutBase}.root

# Input files
set Files = ${input}

# Logfiles. Thanks cshell for this "elegant" syntax to split err and out
set LogFile     = log/mix/${logBase}/y14/mix_${OutBase}.log
set ErrFile     = log/mix/${logBase}/y14/mix_${OutBase}.err

# get relative tree location
set treeFile = `basename $input`
set relativeTreeFile = tree/${treeFile}

echo "Logging output to " $LogFile
echo "Logging errors to " $ErrFile

set arg = "$inputDir $relativeTreeFile $outName $dataType $nEvents $eventsPerTrigger $mixEvents"

qsub -V -q erhiq -l mem=3GB -o $LogFile -e $ErrFile -N auauMixY14 -- ${ExecPath}/submit/qwrap.sh ${ExecPath} $execute $arg

end