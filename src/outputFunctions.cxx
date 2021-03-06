// Implementation of basic functions
// used in the output workflow of
// the jet hadron correlation analysis

#include "outputFunctions.hh"

#include "corrParameters.hh"
#include "histograms.hh"

// to build directories we use boost
#include "boost/filesystem.hpp"

// and some more root stuff
#include "TPaveText.h"
#include "TLatex.h"

// the grid does not have std::to_string() for some ungodly reason
// replacing it here. Simply ostringstream
namespace patch {
  template < typename T > std::string to_string( const T& n )
  {
    std::ostringstream stm ;
    stm << n ;
    return stm.str() ;
  }
}


namespace jetHadron {
  
  // binSelector functionality to set bin information to match
  // current correlations
  void binSelector::SetHistogramBins( TH2F* h ) {
    
    bindEta = h->GetXaxis()->GetNbins();
    dEtaLow = h->GetXaxis()->GetBinLowEdge(1);
    dEtaHigh = h->GetXaxis()->GetBinUpEdge( h->GetXaxis()->GetNbins() );
    bindPhi = h->GetYaxis()->GetNbins();
    dPhiLow = h->GetYaxis()->GetBinLowEdge(1);
    dPhiHigh = h->GetYaxis()->GetBinUpEdge( h->GetYaxis()->GetNbins() );
    
  }
  
  // setting ranges for the histogram
  // if the radius is NOT 0.4, this needs to be used...
  void binSelector::ChangeRadius( double R ) {
    dEtaAcceptanceLow = R - 2.0;
    dEtaAcceptanceHigh = 2.0 - R;
  }
  
  
  // Function used to read in histograms from
  // the files passed in - it returns the correlations,
  // and the number of events, and selects using the centralities,
  // vz bin range, and aj ranges passed in via binSelector
  int ReadInFiles(std::vector<TFile*>& filesIn, std::vector<std::vector<std::vector<std::vector<TH3F*> > > >& leadingCorrelations, std::vector<std::vector<std::vector<std::vector<TH3F*> > > >& subLeadingCorrelations, std::vector<TH3F*>& nEvents, binSelector selector, std::string uniqueID ) {
    
    // loop over all files and all aj, centrality, and vz bins
    // to return a 4D vector of histograms
    for ( int i = 0; i < filesIn.size(); ++i ) {
      // tell the user what is going on
      std::string outMsg = "Reading in file " + patch::to_string(i);
      __OUT(outMsg.c_str() )
      
      // for each file, get the number of events
      nEvents.push_back( (TH3F*) filesIn[i]->Get("nevents") );
      // rename because root can't handle simple crap
      std::string tmpName = "corr_nevents_" + patch::to_string(i);
      nEvents[i]->SetName( tmpName.c_str() );
      
      // push back the vectors
      leadingCorrelations.push_back( std::vector<std::vector<std::vector<TH3F*> > >() );
      subLeadingCorrelations.push_back( std::vector<std::vector<std::vector<TH3F*> > >() );
      
      for ( int j = selector.centLow; j <= selector.centHigh; ++j ) {
        
        int cent_index = j - selector.centLow;
        
        // push back the vectors
        leadingCorrelations[i].push_back( std::vector<std::vector<TH3F*> >() );
        subLeadingCorrelations[i].push_back( std::vector<std::vector<TH3F*> >() );
        
        for ( int k = selector.vzLow; k <= selector.vzHigh; ++k ) {
          
          int vz_index = k - selector.vzLow;
          
          // push back the vectors
          leadingCorrelations[i][cent_index].push_back( std::vector<TH3F*>() );
          subLeadingCorrelations[i][cent_index].push_back( std::vector<TH3F*>() );
          
          for ( int l = selector.ajLow; l <= selector.ajHigh; ++l ) {
            
            int aj_index = l - selector.ajLow;
            
            // build the in-file histogram names
            std::string leadName = "lead_aj_" + patch::to_string(l) + "_cent_" + patch::to_string(j) + "_vz_" + patch::to_string(k);
            std::string subLeadName = "sub_aj_" + patch::to_string(l) + "_cent_" + patch::to_string(j) + "_vz_" + patch::to_string(k);
            
            // check to make sure the file is properly formatted
            if ( !filesIn[i]->Get( leadName.c_str() ) )  {
              __ERR("Can't find histograms - maybe it has mixing correlations not signal?")
              return -1;
            }
            
            // get the correlation histograms
            leadingCorrelations[i][cent_index][vz_index].push_back( (TH3F*) filesIn[i]->Get( leadName.c_str() ) );
            
            // check to make sure it was successful
            if ( !leadingCorrelations[i][cent_index][vz_index][aj_index] ) {
              std::string errorMsg = "Couldn't read in leading correlation: " + patch::to_string(i) + " " + patch::to_string(j) + " " + patch::to_string(k) + " " + patch::to_string(l);
              __ERR( errorMsg.c_str() )
              continue;
            }
            
            
            // if subleading correlations are there, load them in
            if ( filesIn[i]->Get( subLeadName.c_str() ) ) {
              subLeadingCorrelations[i][cent_index][vz_index].push_back( (TH3F*) filesIn[i]->Get( subLeadName.c_str() ) );
            }
            else {
              subLeadingCorrelations[i][cent_index][vz_index].push_back(0x0);
            }
            
            //now differentiate the names by file
            leadName = uniqueID + "_corr_file_" + patch::to_string(i) + "_" + leadName;
            subLeadName = uniqueID +  "_corr_file_" + patch::to_string(i) + "_" + subLeadName;
            
            leadingCorrelations[i][cent_index][vz_index][aj_index]->SetName( leadName.c_str() );
            if ( subLeadingCorrelations[i][cent_index][vz_index][aj_index] ) {
              subLeadingCorrelations[i][cent_index][vz_index][aj_index]->SetName( subLeadName.c_str() );
            }
          }
        }
      }
    }
    return 1;
  }
  
  int ReadInFilesMix(std::vector<TFile*>& filesIn, std::vector<std::vector<std::vector<std::vector<TH3F*> > > >& leadingCorrelations, std::vector<std::vector<std::vector<std::vector<TH3F*> > > >& subLeadingCorrelations, std::vector<TH3F*>& nEvents, binSelector selector, std::string uniqueID ) {
    
    // loop over all files and all aj, centrality, and vz bins
    // to return a 4D vector of histograms
    for ( int i = 0; i < filesIn.size(); ++i ) {
      // tell the user what is going on
      std::string outMsg = "Reading in file " + patch::to_string(i);
      __OUT(outMsg.c_str() )
      
      // for each file, get the number of events
      nEvents.push_back( (TH3F*) filesIn[i]->Get("nevents") );
      // rename because root can't handle simple crap
      std::string tmpName = "mix_nevents_" + patch::to_string(i);
      nEvents[i]->SetName( tmpName.c_str() );
      
      // push back the vectors
      leadingCorrelations.push_back( std::vector<std::vector<std::vector<TH3F*> > >() );
      subLeadingCorrelations.push_back( std::vector<std::vector<std::vector<TH3F*> > >() );
      
      for ( int j = selector.centLow; j <= selector.centHigh; ++j ) {
        
        int cent_index = j - selector.centLow;
        
        // push back the vectors
        leadingCorrelations[i].push_back( std::vector<std::vector<TH3F*> >() );
        subLeadingCorrelations[i].push_back( std::vector<std::vector<TH3F*> >() );
        
        for ( int k = selector.vzLow; k <= selector.vzHigh; ++k ) {
          
          int vz_index = k - selector.vzLow;
          
          // push back the vectors
          leadingCorrelations[i][cent_index].push_back( std::vector<TH3F*>() );
          subLeadingCorrelations[i][cent_index].push_back( std::vector<TH3F*>() );
          
          for ( int l = selector.ajLow; l <= selector.ajHigh; ++l ) {
            
            int aj_index = l - selector.ajLow;
            
            // build the in-file histogram names
            std::string leadName = "mix_lead_aj_" + patch::to_string(l) + "_cent_" + patch::to_string(j) + "_vz_" + patch::to_string(k);
            std::string subLeadName = "mix_sub_aj_" + patch::to_string(l) + "_cent_" + patch::to_string(j) + "_vz_" + patch::to_string(k);
            
            // check to make sure the file is properly formatted
            if ( !filesIn[i]->Get( leadName.c_str() ) ) {
              __ERR("Can't find histograms - maybe it has signal correlations not event mixing?")
              return -1;
            }
            
            // get the correlation histograms
            leadingCorrelations[i][cent_index][vz_index].push_back( (TH3F*) filesIn[i]->Get( leadName.c_str() ) );
            
            // check to make sure it was successful
            if ( !leadingCorrelations[i][cent_index][vz_index][aj_index] ) {
              std::string errorMsg = "Couldn't read in leading correlation: " + patch::to_string(i) + " " + patch::to_string(j) + " " + patch::to_string(k) + " " + patch::to_string(l);
              __ERR( errorMsg.c_str() )
              continue;
            }
            
            // if this is a dijet analysis, get the subleading correlations
            if ( filesIn[i]->Get( subLeadName.c_str() ) ) {
              subLeadingCorrelations[i][cent_index][vz_index].push_back( (TH3F*) filesIn[i]->Get( subLeadName.c_str() ) );
            }
            else {
              subLeadingCorrelations[i][cent_index][vz_index].push_back(0x0);
            }
            
            //now differentiate the names by file
            leadName = uniqueID + "_mix_corr_file_" + patch::to_string(i) + "_" + leadName;
            subLeadName = uniqueID + "_mix_corr_file_" + patch::to_string(i) + "_" + subLeadName;
            
            leadingCorrelations[i][cent_index][vz_index][aj_index]->SetName( leadName.c_str() );
            if ( subLeadingCorrelations[i][cent_index][vz_index][aj_index] ) {
              subLeadingCorrelations[i][cent_index][vz_index][aj_index]->SetName( subLeadName.c_str() );
            }
          }
        }
      }
    }
    return 1;
  }

  
  // Function used to find the weighted center
  // for each pt bin for each file - vector<vector<double> >
  // and also creates pt spectra for each file
  std::vector<std::vector<double> > FindPtBinCenter( std::vector<std::vector<std::vector<std::vector<TH3F*> > > >& correlations, std::vector<TH1F*>& ptSpectra, binSelector selector, std::string uniqueID ) {
    
    // make the returned object 
    std::vector<std::vector<double> > ptBinCenters;
    ptBinCenters.resize( correlations.size() );

    // first we loop over all histograms, project 
    // down to 1D and add together
    ptSpectra.resize( correlations.size() );
    std::vector<std::vector<TH1F*> > ptBinHolder;
    ptBinHolder.resize( correlations.size() );

    for ( int i = 0; i < correlations.size(); ++i ) {
      for  ( int j = 0; j < correlations[i].size(); ++j ) {
        for ( int k = 0; k < correlations[i][j].size(); ++k ) {
          for ( int l = 0; l < correlations[i][j][k].size(); ++l ) {
            for ( int m = 0; m < selector.nPtBins; ++m ) {
              if ( j == 0 && k == 0 && l == 0 ) {
                ptBinHolder[i].resize(selector.nPtBins);
                std::string tmp = "pt_file_" + patch::to_string(i);
                if ( m == 0 )
                  ptSpectra[i] = new TH1F( tmp.c_str(), tmp.c_str(), binsPt, ptLowEdge, ptHighEdge );
              }

              correlations[i][j][k][l]->GetZaxis()->SetRange();
              ptSpectra[i]->Add( (TH1F*) correlations[i][j][k][l]->ProjectionZ() );

              correlations[i][j][k][l]->GetZaxis()->SetRange( selector.ptBinLowEdge(m), selector.ptBinHighEdge(m) );
              if ( !ptBinHolder[i][m] ) {
                ptBinHolder[i][m] = ((TH1F*) ((TH1F*) correlations[i][j][k][l]->ProjectionZ())->Clone());
                std::string tmp = uniqueID + "_pt_file_" + patch::to_string(i) + "_pt_" + patch::to_string(m);
                ptBinHolder[i][m]->SetName( tmp.c_str() );
              }
              else {
                ptBinHolder[i][m]->Add( (TH1F*) correlations[i][j][k][l]->ProjectionZ() );
              }
            }
          }
        }
      }
      
      // now extract the mean values
      ptBinCenters[i].resize(selector.nPtBins);
      for ( int j = 0; j < selector.nPtBins; ++j ) {
        ptBinCenters[i][j] = ptBinHolder[i][j]->GetMean();
        
      }
    }
    
    return ptBinCenters;
  }
  
  // For Correlations
  // Functions to project out the Aj dependence -
  // can either produce a single, Aj independent bin
  // or splits on an ajbin
  void BuildSingleCorrelation( std::vector<std::vector<std::vector<std::vector<TH3F*> > > >& correlations, std::vector<std::vector<std::vector<std::vector<TH2F*> > > >& reducedCorrelations, binSelector selector, std::string uniqueID ) {
    
    // expand the holder
    reducedCorrelations.resize( correlations.size() );
    
    for ( int i = 0; i < correlations.size(); ++i ) {
      reducedCorrelations[i].resize(correlations[i].size() );
      for ( int j = 0; j < correlations[i].size(); ++j ) {
        reducedCorrelations[i][j].resize(correlations[i][j].size() );
        for ( int k = 0; k < correlations[i][j].size(); ++k ) {
          reducedCorrelations[i][j][k].resize( selector.nPtBins );
          for ( int l = 0; l < correlations[i][j][k].size(); ++l ) {
            for ( int m = 0; m < selector.nPtBins; ++m ) {
              
              // select proper pt range
              correlations[i][j][k][l]->GetZaxis()->SetRange( selector.ptBinLowEdge(m), selector.ptBinHighEdge(m) );
              
              if ( !reducedCorrelations[i][j][k][m] ) {
                std::string tmp = uniqueID + "_corr_file_" + patch::to_string(i) + "_cent_" + patch::to_string(j) + "_vz_" + patch::to_string(k);
                reducedCorrelations[i][j][k][m] = (TH2F*) ((TH2F*) correlations[i][j][k][l]->Project3D("YX"))->Clone();
                reducedCorrelations[i][j][k][m]->SetName( tmp.c_str() );
              }
              else {
                reducedCorrelations[i][j][k][m]->Add( (TH2F*) correlations[i][j][k][l]->Project3D("YX") );
              }
            }
          }
        }
      }
    }
  }
  
  void BuildAjSplitCorrelation( std::vector<std::vector<std::vector<std::vector<TH3F*> > > >& correlations, std::vector<std::vector<std::vector<std::vector<TH2F*> > > >& reducedCorrelationsHigh, std::vector<std::vector<std::vector<std::vector<TH2F*> > > >& reducedCorrelationsLow, binSelector selector, int ajBinSplit, std::string uniqueID ) {
    
    // expand the holder
    reducedCorrelationsHigh.resize( correlations.size() );
    reducedCorrelationsLow.resize( correlations.size() );

    for ( int i = 0; i < correlations.size(); ++i ) {
      reducedCorrelationsHigh[i].resize( correlations[i].size() );
      reducedCorrelationsLow[i].resize( correlations[i].size() );
      for ( int j = 0; j < correlations[i].size(); ++j ) {
        reducedCorrelationsHigh[i][j].resize( correlations[i][j].size() );
        reducedCorrelationsLow[i][j].resize( correlations[i][j].size() );
        for ( int k = 0; k < correlations[i][j].size(); ++k ) {
          reducedCorrelationsHigh[i][j][k].resize( selector.nPtBins );
          reducedCorrelationsLow[i][j][k].resize( selector.nPtBins );
          for ( int l = 0; l < correlations[i][j][k].size(); ++l ) {
            for ( int m = 0; m < selector.nPtBins; ++m ) {
              
              // select proper pt range
              correlations[i][j][k][l]->GetZaxis()->SetRange( selector.ptBinLowEdge(m), selector.ptBinHighEdge(m) );
              if ( l >= ajBinSplit ) {
                if ( !reducedCorrelationsHigh[i][j][k][m] ) {
                  std::string tmp = uniqueID +"_corr_aj_low_file_" + patch::to_string(i) + "_cent_" + patch::to_string(j) + "_vz_" + patch::to_string(k);
                  reducedCorrelationsHigh[i][j][k][m] = (TH2F*) ((TH2F*) correlations[i][j][k][l]->Project3D("YX"))->Clone();
                  reducedCorrelationsHigh[i][j][k][m]->SetName( tmp.c_str() );
                }
                else {
                  reducedCorrelationsHigh[i][j][k][m]->Add( (TH2F*) correlations[i][j][k][l]->Project3D("YX") );
                }
              }
              else {
                if ( !reducedCorrelationsLow[i][j][k][m] ) {
                  std::string tmp = uniqueID + "_corr_aj_high_file_" + patch::to_string(i) + "_cent_" + patch::to_string(j) + "_vz_" + patch::to_string(k);
                  reducedCorrelationsLow[i][j][k][m] = (TH2F*) ((TH2F*) correlations[i][j][k][l]->Project3D("YX"))->Clone();
                  reducedCorrelationsLow[i][j][k][m]->SetName( tmp.c_str() );
                }
                else {
                  reducedCorrelationsLow[i][j][k][m]->Add( (TH2F*) correlations[i][j][k][l]->Project3D("YX") );
                }
              }
              
            }
          }
        }
      }
    }
  }
  
  // Averages over all vz and centralities
  // to show uncorrected signals
  std::vector<std::vector<TH2F*> > AverageCorrelations( std::vector<std::vector<std::vector<std::vector<TH2F*> > > >& correlations, binSelector selector, std::string uniqueID ) {
    
    // build the initial holder
    std::vector<std::vector<TH2F*> > averagedCorrelations;
    averagedCorrelations.resize( correlations.size() );
    
    for ( int i = 0; i < correlations.size(); ++i ) {
      averagedCorrelations[i].resize(selector.nPtBins);
      for ( int j = 0; j < correlations[i].size(); ++j ) {
        for ( int k = 0; k < correlations[i][j].size(); ++k ) {
          for ( int l = 0; l < correlations[i][j][k].size(); ++l ) {
            
            if ( !averagedCorrelations[i][l] ) {
              std::string tmp = uniqueID + "_averaged_file_" + patch::to_string(i) + "_pt_" + patch::to_string(l);
              if ( TString(correlations[i][j][k][l]->GetName()).Contains("low") ) {
                tmp = uniqueID + "_averaged_aj_low_file_" + patch::to_string(i) + "_pt_" + patch::to_string(l);
              }
              if ( TString(correlations[i][j][k][l]->GetName()).Contains("high") ) {
                tmp = uniqueID + "_averaged_aj_high_file_" + patch::to_string(i) + "_pt_" + patch::to_string(l);
              }
              
              averagedCorrelations[i][l] = ((TH2F*) correlations[i][j][k][l]->Clone());
              averagedCorrelations[i][l]->SetName( tmp.c_str() );
            }
            else {
              
              averagedCorrelations[i][l]->Add( correlations[i][j][k][l] );
            }
          }
        }
      }
    }
    
    return averagedCorrelations;
  }
  
  
  // For Mixed Events
  // Used to recombine Aj and split in pt
  // to give 2D projections we can turn use
  // to correct the correlations
  std::vector<std::vector<std::vector<std::vector<TH2F*> > > > BuildMixedEvents( std::vector<std::vector<std::vector<std::vector<TH3F*> > > >& mixedEvents, binSelector selector, std::string uniqueID ) {
    
    std::vector<std::vector<std::vector<std::vector<TH2F*> > > > finalMixedEvents;
    finalMixedEvents.resize( mixedEvents.size() );
    
    for ( int i = 0; i < mixedEvents.size(); ++i ) {
      finalMixedEvents[i].resize( mixedEvents[i].size() );
      for ( int j = 0; j < mixedEvents[i].size(); ++j ) {
        finalMixedEvents[i][j].resize( mixedEvents[i][j].size() );
        for ( int k = 0; k < mixedEvents[i][j].size(); ++k ) {
          finalMixedEvents[i][j][k].resize( selector.nPtBins );
          for ( int l = 0; l < mixedEvents[i][j][k].size(); ++l ) {
            for ( int m = 0; m < selector.nPtBins; ++m ){
              
              // select the proper pt range for each histogram
              mixedEvents[i][j][k][l]->GetZaxis()->SetRange( selector.ptBinLowEdge(m), selector.ptBinHighEdge(m) );
              
              if ( !finalMixedEvents[i][j][k][m] ) {
                finalMixedEvents[i][j][k][m] = (TH2F*) ((TH2F*) mixedEvents[i][j][k][l]->Project3D("YX"))->Clone();
                std::string tmp = uniqueID + "_mix_file_" + patch::to_string(i) + "_cent_" + patch::to_string(j) + "_vz_" + patch::to_string(k) + "_pt_" + patch::to_string(m);
                finalMixedEvents[i][j][k][m]->SetName( tmp.c_str() );
              }
              else {
                finalMixedEvents[i][j][k][m]->Add( (TH2F*) mixedEvents[i][j][k][l]->Project3D("YX") );
              }
            }
          }
        }
      }
    }
    
    return finalMixedEvents;
  }

  
  // Used to average the mixed event data to help
  // with the lower statistics
  std::vector<std::vector<TH2F*> > RecombineMixedEvents( std::vector<std::vector<std::vector<std::vector<TH3F*> > > >& mixedEvents, binSelector selector, std::string uniqueID ) {
    
    // create the return object
    std::vector<std::vector<TH2F*> > combinedMixedEvents;
    combinedMixedEvents.resize( mixedEvents.size() );
    
    for ( int i = 0; i < mixedEvents.size(); ++i ) {
      combinedMixedEvents[i].resize( 3 );
      
      for ( int j = 0; j < mixedEvents[i].size(); ++j ) {
        for ( int k = 0; k < mixedEvents[i][j].size(); ++k ) {
          for ( int l = 0; l < mixedEvents[i][j][k].size(); ++l ) {
            for ( int m = 0; m < selector.nPtBins; ++m ) {
              
              mixedEvents[i][j][k][l]->GetZaxis()->SetRange( selector.ptBinLowEdge(m), selector.ptBinHighEdge(m) );
              
              if ( m <= 2 ) {
                if ( !combinedMixedEvents[i][m] ) {
                  combinedMixedEvents[i][m] = (TH2F*) ((TH2F*) mixedEvents[i][j][k][l]->Project3D("YX"))->Clone();
                  std::string tmp = uniqueID + "_mix_file_" + patch::to_string(i) + "_pt_" + patch::to_string(m);
                  combinedMixedEvents[i][m]->SetName( tmp.c_str() );
                }
                else {
                  combinedMixedEvents[i][m]->Add( (TH2F*) mixedEvents[i][j][k][l]->Project3D("YX") );
                }
              }
              else {
                if ( !combinedMixedEvents[i][2] ) {
                  combinedMixedEvents[i][2] = (TH2F*) ((TH2F*) mixedEvents[i][j][k][l]->Project3D("YX"))->Clone();
                  std::string tmp = uniqueID + "_mix_file_" + patch::to_string(i) + "_pt_" + patch::to_string(m);
                  combinedMixedEvents[i][2]->SetName( tmp.c_str() );
                }
                else {
                  combinedMixedEvents[i][2]->Add( (TH2F*) mixedEvents[i][j][k][l]->Project3D("YX") );
                }
              }
            }
          }
        }
      }
    }
    
    return combinedMixedEvents;
  }
  
  // Used to partially recomine over Vz bins
  // but leaves centrality untouched
  std::vector<std::vector<std::vector<TH2F*> > > PartialRecombineMixedEvents( std::vector<std::vector<std::vector<std::vector<TH3F*> > > >& mixedEvents, binSelector selector, std::string uniqueID  ) {
    
    // create return object
    std::vector<std::vector<std::vector<TH2F*> > > combinedMixedEvents;
    combinedMixedEvents.resize( mixedEvents.size() );
    
    for ( int i = 0; i < mixedEvents.size(); ++i ) {
      combinedMixedEvents[i].resize( mixedEvents[i].size() );
      
      for ( int j = 0; j < mixedEvents[i].size(); ++j ) {
        combinedMixedEvents[i][j].resize( 3 );
        
        for ( int k = 0; k < mixedEvents[i][j].size(); ++k ) {
          for ( int l = 0; l < mixedEvents[i][j][k].size(); ++l ) {
            for ( int m = 0; m < selector.nPtBins; ++m ) {
              
              mixedEvents[i][j][k][l]->GetZaxis()->SetRange( selector.ptBinLowEdge(m), selector.ptBinHighEdge(m) );
              
              if ( m <= 2 ) {
                if ( !combinedMixedEvents[i][j][m] ) {
                  combinedMixedEvents[i][j][m] = (TH2F*) ((TH2F*) mixedEvents[i][j][k][l]->Project3D("YX"))->Clone();
                  std::string tmp = uniqueID + "_mix_file_" + patch::to_string(i) + "_cent_" + patch::to_string(j) + "_pt_" + patch::to_string(m);
                  combinedMixedEvents[i][j][m]->SetName( tmp.c_str() );
                }
                else {
                  combinedMixedEvents[i][j][m]->Add( (TH2F*) mixedEvents[i][j][k][l]->Project3D("YX") );
                }
              }
              else {
                if ( !combinedMixedEvents[i][j][2] ) {
                  combinedMixedEvents[i][j][2] = (TH2F*) ((TH2F*) mixedEvents[i][j][k][l]->Project3D("YX"))->Clone();
                  std::string tmp = uniqueID + "_mix_file_" + patch::to_string(i) + "_cent_" + patch::to_string(j) + "_pt_" + patch::to_string(m);
                  combinedMixedEvents[i][j][2]->SetName( tmp.c_str() );
                }
                else {
                  combinedMixedEvents[i][j][2]->Add( (TH2F*) mixedEvents[i][j][k][l]->Project3D("YX") );
                }
              }
            }
          }
        }
      }
    }

    return combinedMixedEvents;
  }
  
  // Used to normalize mixed event histograms so
  // that the maximum bin content = 1
  // version for both the independent mixed events and the weighed averages
  void ScaleMixedEvents( std::vector<std::vector<TH2F*> >& mixedEvents ) {
    
    // scale each histogram
    for ( int i = 0; i < mixedEvents.size(); ++i ) {
      for ( int j = 0; j < mixedEvents[i].size(); ++j ) {
        if ( mixedEvents[i][j]->GetEntries() ) {

          TH1F* tmp = (TH1F*) mixedEvents[i][j]->ProjectionX();

          tmp->Scale( 1.0 / (double) mixedEvents[i][j]->GetYaxis()->GetNbins() );

          mixedEvents[i][j]->Scale( 1.0 / tmp->GetMaximum() );
          delete tmp;
        }
      }
    }
  }
  
  void ScaleMixedEvents( std::vector<std::vector<std::vector<std::vector<TH2F*> > > >& mixedEvents ) {
    
    // scale each histogram
    for ( int i = 0; i < mixedEvents.size(); ++i ) {
      for ( int j = 0; j < mixedEvents[i].size(); ++j ) {
        for ( int k = 0; k < mixedEvents[i][j].size(); ++k ) {
          for ( int l = 0; l < mixedEvents[i][j][k].size(); ++l ) {
            if ( mixedEvents[i][j][k][l]->GetEntries() ) {
              
              TH1F* tmp = (TH1F*) mixedEvents[i][j][k][l]->ProjectionX();
              tmp->Scale( 1.0 / (double) mixedEvents[i][j][k][l]->GetYaxis()->GetNbins() );
              
              mixedEvents[i][j][k][l]->Scale( 1.0 / tmp->GetMaximum() );
              delete tmp;
            }
          }
        }
      }
    }
  }
  
  // Used to perform the mixed event division
  // And add up everything into a 2D structure
  // only keeping differntial in file and Pt
  // Has a version for both the averaged and non
  // averaged event mixing
  std::vector<std::vector<TH2F*> > EventMixingCorrection( std::vector<std::vector<std::vector<std::vector<TH2F*> > > >& correlations, std::vector<std::vector<std::vector<std::vector<TH2F*> > > >& mixedEvents, binSelector selector, std::string uniqueID ) {
    
    // create holder for the output
    std::vector<std::vector<TH2F*> > correctedCorrelations;
    correctedCorrelations.resize(correlations.size() );
    
    for ( int i = 0; i < correlations.size(); ++i ) {
      correctedCorrelations[i].resize( selector.nPtBins );
      for ( int j = 0; j < correlations[i].size(); ++j ) {
        for ( int k = 0; k < correlations[i][j].size(); ++k ) {
          for ( int l = 0; l < correlations[i][j][k].size(); ++l ) {
            
            if ( !correctedCorrelations[i][l] ) {
              std::string tmp = uniqueID + "_corrected_file_" + patch::to_string(i) + "_pt_" + patch::to_string(l);
              if ( TString(correlations[i][j][k][l]->GetName()).Contains("low") ) {
                tmp = uniqueID + "_corrected_aj_low_file_" + patch::to_string(i) + "_pt_" + patch::to_string(l);
              }
              if ( TString(correlations[i][j][k][l]->GetName()).Contains("high") ) {
                tmp = uniqueID + "_corrected_aj_high_file_" + patch::to_string(i) + "_pt_" + patch::to_string(l);
              }
              
              if ( correlations[i][j][k][l]->GetEntries() && mixedEvents[i][j][k][l]->GetEntries() ) {
                TH2F* hTmp = ((TH2F*) correlations[i][j][k][l]->Clone());
                hTmp->Divide( mixedEvents[i][j][k][l] );
                correctedCorrelations[i][l] = (TH2F*) hTmp->Clone();
                correctedCorrelations[i][l]->SetName( tmp.c_str() );
              }
            }
            else {
              if ( correlations[i][j][k][l]->GetEntries() && mixedEvents[i][j][k][l]->GetEntries() ) {
                TH2F* hTmp = ((TH2F*) correlations[i][j][k][l]->Clone());
                hTmp->Divide( mixedEvents[i][j][k][l] );
                correctedCorrelations[i][l]->Add( hTmp );
              }
            }
          }
        }
      }
    }
    return correctedCorrelations;
  }
  
  // performs event mixing w/ mixed events
  // differential in pt & centrality
  // averaged over vz
  std::vector<std::vector<TH2F*> > EventMixingCorrection( std::vector<std::vector<std::vector<std::vector<TH2F*> > > >& correlations, std::vector<std::vector<std::vector<TH2F*> > >& mixedEvents, binSelector selector, std::string uniqueID ) {
    
    // create holder for the output
    std::vector<std::vector<TH2F*> > correctedCorrelations;
    correctedCorrelations.resize(correlations.size() );
    
    for ( int i = 0; i < correlations.size(); ++i ) {
      correctedCorrelations[i].resize( selector.nPtBins );
      for ( int j = 0; j < correlations[i].size(); ++j ) {
        for ( int k = 0; k < correlations[i][j].size(); ++k ) {
          for ( int l = 0; l < correlations[i][j][k].size(); ++l ) {
            if ( !correctedCorrelations[i][l] ) {
              std::string tmp = uniqueID + "_corrected_file_" + patch::to_string(i) + "_pt_" + patch::to_string(l);
              if ( TString(correlations[i][j][k][l]->GetName()).Contains("low") ) {
                tmp = uniqueID + "_corrected_aj_low_file_" + patch::to_string(i) + "_pt_" + patch::to_string(l);
              }
              if ( TString(correlations[i][j][k][l]->GetName()).Contains("high") ) {
                tmp = uniqueID + "_corrected_aj_high_file_" + patch::to_string(i) + "_pt_" + patch::to_string(l);
              }
              
              if ( correlations[i][j][k][l]->GetEntries() ) {
                TH2F* hTmp = ((TH2F*) correlations[i][j][k][l]->Clone());
                if ( l <= 2 && mixedEvents[i][j][l]->GetEntries() ) {
                  hTmp->Divide( mixedEvents[i][j][l] );
                }
                else if ( mixedEvents[i][j][2]->GetEntries() )  {
                  hTmp->Divide( mixedEvents[i][j][2] );
                }
                else {
                  __ERR("Did not have any mixed event data to correct with")
                  continue;
                }
                correctedCorrelations[i][l] = (TH2F*) hTmp->Clone();
                correctedCorrelations[i][l]->SetName( tmp.c_str() );
              }
            }
            else {
              if ( correlations[i][j][k][l]->GetEntries() ) {
                TH2F* hTmp = ((TH2F*) correlations[i][j][k][l]->Clone());
                if ( l <= 2 && mixedEvents[i][j][l]->GetEntries() ) {
                  hTmp->Divide( mixedEvents[i][j][l] );
                }
                else if ( mixedEvents[i][j][2]->GetEntries() )  {
                  hTmp->Divide( mixedEvents[i][j][2] );
                }
                else {
                  __ERR("Did not have any mixed event data to correct with")
                  continue;
                }
                correctedCorrelations[i][l]->Add( hTmp );
              }
            }
          }
        }
      }
    }
    return correctedCorrelations;
  }

  // performs event mixing with mixed events
  // only differentiated w/ pt
  // averaged over centrality & vz
  std::vector<std::vector<TH2F*> > EventMixingCorrection( std::vector<std::vector<std::vector<std::vector<TH2F*> > > >& correlations, std::vector<std::vector<TH2F*> >& mixedEvents, binSelector selector, std::string uniqueID ) {
    
    // create holder for the output
    std::vector<std::vector<TH2F*> > correctedCorrelations;
    correctedCorrelations.resize(correlations.size() );
    
    for ( int i = 0; i < correlations.size(); ++i ) {
      correctedCorrelations[i].resize( selector.nPtBins );
      for ( int j = 0; j < correlations[i].size(); ++j ) {
        for ( int k = 0; k < correlations[i][j].size(); ++k ) {
          for ( int l = 0; l < correlations[i][j][k].size(); ++l ) {
            if ( !correctedCorrelations[i][l] ) {
              std::string tmp = uniqueID + "_corrected_file_" + patch::to_string(i) + "_pt_" + patch::to_string(l);
              if ( TString(correlations[i][j][k][l]->GetName()).Contains("low") ) {
                tmp = uniqueID + "_corrected_aj_low_file_" + patch::to_string(i) + "_pt_" + patch::to_string(l);
              }
              if ( TString(correlations[i][j][k][l]->GetName()).Contains("high") ) {
                tmp = uniqueID + "_corrected_aj_high_file_" + patch::to_string(i) + "_pt_" + patch::to_string(l);
              }
              
              if ( correlations[i][j][k][l]->GetEntries() ) {
                TH2F* hTmp = ((TH2F*) correlations[i][j][k][l]->Clone());
                if ( l <= 2 && mixedEvents[i][l]->GetEntries() ) {
                  hTmp->Divide( mixedEvents[i][l] );
                }
                else if ( mixedEvents[i][2]->GetEntries() )  {
                  hTmp->Divide( mixedEvents[i][2] );
                }
                else {
                  __ERR("Did not have any mixed event data to correct with")
                  continue;
                }
                correctedCorrelations[i][l] = (TH2F*) hTmp->Clone();
                correctedCorrelations[i][l]->SetName( tmp.c_str() );
              }
            }
            else {
              if ( correlations[i][j][k][l]->GetEntries() ) {
                TH2F* hTmp = ((TH2F*) correlations[i][j][k][l]->Clone());
                if ( l <= 2 && mixedEvents[i][l]->GetEntries() ) {
                  hTmp->Divide( mixedEvents[i][l] );
                }
                else if ( mixedEvents[i][2]->GetEntries() )  {
                  hTmp->Divide( mixedEvents[i][2] );
                }
                else {
                  __ERR("Did not have any mixed event data to correct with")
                  continue;
                }
                correctedCorrelations[i][l]->Add( hTmp );
              }
            }
          }
        }
      }
    }
    return correctedCorrelations;
  }
  
  // Used to extract 1D projections from
  // the 2D histograms - allows for setting
  // ranges for the projection ( e.g. projecting
  // only the near side of the dPhi range in a dEta
  // projection )
  std::vector<std::vector<TH1F*> > ProjectDphi( std::vector<std::vector<TH2F*> >& correlation2d, binSelector selector, std::string uniqueID ) {
    
    // build the return vector
    std::vector<std::vector<TH1F*> > projections;
    projections.resize( correlation2d.size() );
    
    // now loop over every 2d histogram and project
    for ( int i = 0; i < correlation2d.size(); ++i ) {
      projections[i].resize( correlation2d[i].size() );
      for ( int j = 0; j < correlation2d[i].size(); ++j ) {
        
        std::cout<<"projecting dphi - file: "<< i << " pt bin: "<< j << std::endl;
        std::cout<<"projection bins ( in deta ): "<<correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_eta_bound_low ) << " - " << correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_eta_bound_high ) << std::endl;
        std::cout<<"projection range: " << correlation2d[i][j]->GetXaxis()->GetBinLowEdge(correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_eta_bound_low )) << " - " << correlation2d[i][j]->GetXaxis()->GetBinUpEdge(correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_eta_bound_high ) ) << std::endl;
        
        //do quick resets
        correlation2d[i][j]->GetXaxis()->SetRange();
        correlation2d[i][j]->GetYaxis()->SetRange();
        
        // new name for the projection
        std::string tmp = uniqueID + "_dphi_file_" + patch::to_string(i) + "_pt_" + patch::to_string(j);
        
        correlation2d[i][j]->GetXaxis()->SetRangeUser( selector.phi_projection_eta_bound_low, selector.phi_projection_eta_bound_high );
        
        projections[i][j] = (TH1F*) correlation2d[i][j]->ProjectionY();
        projections[i][j]->SetName( tmp.c_str() );
        
        correlation2d[i][j]->GetXaxis()->SetRange();

        
      }
    }
    
    return projections;
  }
  
  // returns the inner section ( deta < some value )
  // minus the outside area, defined by edges
  // the subtraction is done in three regions
  // region 1 = [edge1, edge2)
  // region 2 = [edge2, edge3]
  // region 3 = (edge3, edge4]
  // result = region 2 - (region 1 + region 3)
  std::vector<std::vector<TH1F*> > ProjectDphiNearMinusFar( std::vector<std::vector<TH2F*> >& correlation2d, binSelector selector, std::string uniqueID ) {
    
    // build the return vector
    std::vector<std::vector<TH1F*> > projections;
    projections.resize( correlation2d.size() );
    
    // now loop over every 2d histogram and project
    for ( int i = 0; i < correlation2d.size(); ++i ) {
      projections[i].resize( correlation2d[i].size() );
      for ( int j = 0; j < correlation2d[i].size(); ++j ) {
        
        std::cout<<"projecting dphi near minus far - file: "<< i << " pt bin: "<< j << std::endl;
        std::cout<<"projection bins full range ( in deta ): "<<correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions[0] ) << " - " << correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions[3] ) << std::endl;
        std::cout<<"projection bins inner range ( in deta ): "<<correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions[1] ) << " - " << correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions[2] ) << std::endl;
        std::cout<<"projection range (full): " << correlation2d[i][j]->GetXaxis()->GetBinLowEdge(correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions[0] )) << " - " << correlation2d[i][j]->GetXaxis()->GetBinUpEdge(correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions[3] ) ) << std::endl;
        std::cout<<"projection range (inner): " << correlation2d[i][j]->GetXaxis()->GetBinLowEdge(correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions[1] )) << " - " << correlation2d[i][j]->GetXaxis()->GetBinUpEdge(correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions[2] ) ) << std::endl;
        
        

        //do quick resets
        correlation2d[i][j]->GetXaxis()->SetRange();
        correlation2d[i][j]->GetYaxis()->SetRange();
        
        // new name for the projection
        std::string tmp = uniqueID + "_dphi_file_" + patch::to_string(i) + "_pt_" + patch::to_string(j);
        
        // now get the bins
        double region1Low = correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions[0] );
        double region1High = correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions[1] ) - 1;
        double region2Low = correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions[1] );
        double region2High = correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions[2] );
        double region3Low = correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions[2] ) + 1;
        double region3High = correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions[3] );

        
        // do some sanity checking
        if ( region3High < region3Low || region2High < region2Low || region1High < region1Low ) {
          __ERR("Can't project - high edge less than low edge for one of the projection regions")
          continue;
        }
        
        // now do the projections
        correlation2d[i][j]->GetXaxis()->SetRange( region2Low, region2High );
        projections[i][j] = (TH1F*) correlation2d[i][j]->ProjectionY();
        projections[i][j]->SetName( tmp.c_str() );
        
        // build the subtraction histogram
        correlation2d[i][j]->GetXaxis()->SetRange( region1Low, region1High );
        TH1F* sub_tmp = (TH1F*) ((TH1F*)  correlation2d[i][j]->ProjectionY())->Clone();
        correlation2d[i][j]->GetXaxis()->SetRange( region3Low, region3High );
        sub_tmp->Add( (TH1F*) correlation2d[i][j]->ProjectionY() );
        
        // scale the subtraction histogram by the relative number of bins
        sub_tmp->Scale( (region2High-region2Low + 1 )/( (region1High-region1Low + 1 ) + (region3High - region3Low + 1 ) ) );
        
        // subtract
        projections[i][j]->Add( sub_tmp, -1 );
        
      }
    }
    return projections;
  }
  
  // this returns both near and far individually
  void ProjectDphiNearMinusFar( std::vector<std::vector<TH2F*> >& correlation2d, std::vector<std::vector<TH1F*> >& near, std::vector<std::vector<TH1F*> >& far, binSelector selector, std::string uniqueID ) {
    
    
    // scale the return histograms
    near.resize( correlation2d.size() );
    far.resize( correlation2d.size() );
    
    // now loop over every 2d histogram and project
    for ( int i = 0; i < correlation2d.size(); ++i ) {
      near[i].resize( correlation2d[i].size() );
      far[i].resize( correlation2d[i].size() );
      
      for ( int j = 0; j < correlation2d[i].size(); ++j ) {
        
        std::cout<<"projecting dphi near minus far - file: "<< i << " pt bin: "<< j << std::endl;
        std::cout<<"projection bins full range ( in deta ): "<<correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions[0] ) << " - " << correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions[3] ) << std::endl;
        std::cout<<"projection bins inner range ( in deta ): "<<correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions[1] ) << " - " << correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions[2] ) << std::endl;
        std::cout<<"projection range (full): " << correlation2d[i][j]->GetXaxis()->GetBinLowEdge(correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions[0] )) << " - " << correlation2d[i][j]->GetXaxis()->GetBinUpEdge(correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions[3] ) ) << std::endl;
        std::cout<<"projection range (inner): " << correlation2d[i][j]->GetXaxis()->GetBinLowEdge(correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions[1] )) << " - " << correlation2d[i][j]->GetXaxis()->GetBinUpEdge(correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions[2] ) ) << std::endl;
        
        
        
        //do quick resets
        correlation2d[i][j]->GetXaxis()->SetRange();
        correlation2d[i][j]->GetYaxis()->SetRange();
        
        // new name for the projection
        std::string tmpNear = uniqueID + "_near_dphi_file_" + patch::to_string(i) + "_pt_" + patch::to_string(j);
        std::string tmpFar = uniqueID + "_far_dphi_file_" + patch::to_string(i) + "_pt_" + patch::to_string(j);
        
        // now get the bins
        // now get the bins
        double region1Low = correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions[0] );
        double region1High = correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions[1] ) - 1;
        double region2Low = correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions[1] );
        double region2High = correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions[2] );
        double region3Low = correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions[2] ) + 1;
        double region3High = correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions[3] );
        
        
        // do some sanity checking
        if ( region3High < region3Low || region2High < region2Low || region1High < region1Low ) {
          __ERR("Can't project - high edge less than low edge for one of the projection regions")
          continue;
        }
        
        
        // now do the projections
        correlation2d[i][j]->GetXaxis()->SetRange( region2Low, region2High );
        near[i][j] = (TH1F*) correlation2d[i][j]->ProjectionY();
        near[i][j]->SetName( tmpNear.c_str() );
        
        // for the far as well
        correlation2d[i][j]->GetXaxis()->SetRange( region1Low, region1High );
        far[i][j] = (TH1F*) correlation2d[i][j]->ProjectionY();
        far[i][j]->SetName( tmpFar.c_str() );
        correlation2d[i][j]->GetXaxis()->SetRange( region3Low, region3High );
        far[i][j]->Add( (TH1F*) correlation2d[i][j]->ProjectionY() );
        
        // scale the subtraction histogram by the relative number of bins
        far[i][j]->Scale( (region2High-region2Low + 1 )/( (region1High-region1Low + 1 ) + (region3High - region3Low + 1) ) );

        
      }
    }
  }
  
  std::vector<std::vector<TH1F*> > ProjectDeta( std::vector<std::vector<TH2F*> >& correlation2d, binSelector selector, std::string uniqueID ) {
    // build the return vector
    std::vector<std::vector<TH1F*> > projections;
    projections.resize( correlation2d.size() );
    
    // now loop over every 2d histogram and project
    for ( int i = 0; i < correlation2d.size(); ++i ) {
      projections[i].resize( correlation2d[i].size() );
      for ( int j = 0; j < correlation2d[i].size(); ++j ) {
        
        //do quick resets
        correlation2d[i][j]->GetXaxis()->SetRange();
        correlation2d[i][j]->GetYaxis()->SetRange();
        
        std::cout<<"projecting dEta - file: "<< i << " pt bin: "<< j << std::endl;
        std::cout<<"projection bins ( in dphi ): "<<correlation2d[i][j]->GetYaxis()->FindBin( selector.eta_projection_phi_bound_low ) << " - " << correlation2d[i][j]->GetYaxis()->FindBin( selector.eta_projection_phi_bound_high ) << std::endl;
        std::cout<<"projection range: " << correlation2d[i][j]->GetYaxis()->GetBinLowEdge(correlation2d[i][j]->GetYaxis()->FindBin( selector.eta_projection_phi_bound_low )) << " - " << correlation2d[i][j]->GetYaxis()->GetBinUpEdge(correlation2d[i][j]->GetYaxis()->FindBin( selector.eta_projection_phi_bound_high ) ) << std::endl;
        
        // new name for the projection
        std::string tmp = uniqueID + "_deta_file_" + patch::to_string(i) + "_pt_" + patch::to_string(j);
        
        correlation2d[i][j]->GetYaxis()->SetRangeUser( selector.eta_projection_phi_bound_low, selector.eta_projection_phi_bound_high );
        
        projections[i][j] = (TH1F*) correlation2d[i][j]->ProjectionX();
        projections[i][j]->SetName( tmp.c_str() );
        
        correlation2d[i][j]->GetYaxis()->SetRange();

        
      }
    }
    
    return projections;
  }
  
  // With more bins for our extended range error
  std::vector<std::vector<TH1F*> > ProjectDphiNearMinusFarExtended( std::vector<std::vector<TH2F*> >& correlation2d, binSelector selector, std::string uniqueID ) {
    
    // build the return vector
    std::vector<std::vector<TH1F*> > projections;
    projections.resize( correlation2d.size() );
    
    // now loop over every 2d histogram and project
    for ( int i = 0; i < correlation2d.size(); ++i ) {
      projections[i].resize( correlation2d[i].size() );
      for ( int j = 0; j < correlation2d[i].size(); ++j ) {
        
        std::cout<<"projecting dphi near minus far with extended range - file: "<< i << " pt bin: "<< j << std::endl;
        std::cout<<"projection bins full range ( in deta ): "<<correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions_extended[0] ) << " - " << correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions_extended[3] ) << std::endl;
        std::cout<<"projection bins inner range ( in deta ): "<<correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions_extended[1] ) << " - " << correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions_extended[2] ) << std::endl;
        std::cout<<"projection range (full): " << correlation2d[i][j]->GetXaxis()->GetBinLowEdge(correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions_extended[0] )) << " - " << correlation2d[i][j]->GetXaxis()->GetBinUpEdge(correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions_extended[3] ) ) << std::endl;
        std::cout<<"projection range (inner): " << correlation2d[i][j]->GetXaxis()->GetBinLowEdge(correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions_extended[1] )) << " - " << correlation2d[i][j]->GetXaxis()->GetBinUpEdge(correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions_extended[2] ) ) << std::endl;
        
        //do quick resets
        correlation2d[i][j]->GetXaxis()->SetRange();
        correlation2d[i][j]->GetYaxis()->SetRange();
        
        // new name for the projection
        std::string tmp = uniqueID + "_dphi_file_" + patch::to_string(i) + "_pt_" + patch::to_string(j);
        
        // now get the bins
        double region1Low = correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions_extended[0] );
        double region1High = correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions_extended[1] ) - 1;
        double region2Low = correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions_extended[1] );
        double region2High = correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions_extended[2] );
        double region3Low = correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions_extended[2] ) + 1;
        double region3High = correlation2d[i][j]->GetXaxis()->FindBin( selector.phi_projection_subtraction_regions_extended[3] );
        
        
        // do some sanity checking
        if ( region3High < region3Low || region2High < region2Low || region1High < region1Low ) {
          __ERR("Can't project - high edge less than low edge for one of the projection regions")
          continue;
        }
        
        // now do the projections
        correlation2d[i][j]->GetXaxis()->SetRange( region2Low, region2High );
        projections[i][j] = (TH1F*) correlation2d[i][j]->ProjectionY();
        projections[i][j]->SetName( tmp.c_str() );
        
        // build the subtraction histogram
        correlation2d[i][j]->GetXaxis()->SetRange( region1Low, region1High );
        TH1F* sub_tmp = (TH1F*) ((TH1F*)  correlation2d[i][j]->ProjectionY())->Clone();
        correlation2d[i][j]->GetXaxis()->SetRange( region3Low, region3High );
        sub_tmp->Add( (TH1F*) correlation2d[i][j]->ProjectionY() );
        
        // scale the subtraction histogram by the relative number of bins
        sub_tmp->Scale( (region2High-region2Low + 1 )/( (region1High-region1Low + 1 ) + (region3High - region3Low + 1 ) ) );
        
        // subtract
        projections[i][j]->Add( sub_tmp, -1 );
        
      }
    }
    return projections;
  }

  std::vector<std::vector<TH1F*> > ProjectDetaExtended( std::vector<std::vector<TH2F*> >& correlation2d, binSelector selector, std::string uniqueID ) {
    // build the return vector
    std::vector<std::vector<TH1F*> > projections;
    projections.resize( correlation2d.size() );
    
    // now loop over every 2d histogram and project
    for ( int i = 0; i < correlation2d.size(); ++i ) {
      projections[i].resize( correlation2d[i].size() );
      for ( int j = 0; j < correlation2d[i].size(); ++j ) {
        
        //do quick resets
        correlation2d[i][j]->GetXaxis()->SetRange();
        correlation2d[i][j]->GetYaxis()->SetRange();
        
        std::cout<<"projecting dEta with extended range - file: "<< i << " pt bin: "<< j << std::endl;
        std::cout<<"projection bins ( in dphi ): "<<correlation2d[i][j]->GetYaxis()->FindBin( selector.eta_projection_phi_bound_low_extended ) << " - " << correlation2d[i][j]->GetYaxis()->FindBin( selector.eta_projection_phi_bound_high_extended ) << std::endl;
        std::cout<<"projection range: " << correlation2d[i][j]->GetYaxis()->GetBinLowEdge(correlation2d[i][j]->GetYaxis()->FindBin( selector.eta_projection_phi_bound_low_extended )) << " - " << correlation2d[i][j]->GetYaxis()->GetBinUpEdge(correlation2d[i][j]->GetYaxis()->FindBin( selector.eta_projection_phi_bound_high_extended ) ) << std::endl;
        
        // new name for the projection
        std::string tmp = uniqueID + "_deta_file_" + patch::to_string(i) + "_pt_" + patch::to_string(j);
        
        correlation2d[i][j]->GetYaxis()->SetRangeUser( selector.eta_projection_phi_bound_low_extended, selector.eta_projection_phi_bound_high_extended );
        
        projections[i][j] = (TH1F*) correlation2d[i][j]->ProjectionX();
        projections[i][j]->SetName( tmp.c_str() );
        
        correlation2d[i][j]->GetYaxis()->SetRange();
        
        
      }
    }
    
    return projections;
  }
  
  
  // Normalizes 1D histograms based on what
  // type of analysis they are
  void Normalize1D( std::vector<std::vector<TH1F*> >& histograms, std::vector<TH3F*>& nEvents ) {
    
    for ( int i = 0; i < histograms.size(); ++i ) {
      for ( int j = 0; j < histograms[i].size(); ++j ) {
        histograms[i][j]->Scale( 1.0 / histograms[i][j]->GetXaxis()->GetBinWidth(1) );
        histograms[i][j]->Scale( 1.0 / (double) nEvents[i]->GetEntries() );
      }
    }
    
  }
  
  // need to know what
  void Normalize1DAjSplit( std::vector<std::vector<TH1F*> >& histograms, std::vector<TH3F*>& nEvents, int ajBinLow, int ajBinHigh ) {
    
    for ( int i = 0; i < histograms.size(); ++i ) {
      nEvents[i]->GetXaxis()->SetRange( ajBinLow, ajBinHigh );
      for ( int j = 0; j < histograms[i].size(); ++j ) {

        histograms[i][j]->Scale( 1.0 /histograms[i][j]->GetXaxis()->GetBinWidth(1) );
        histograms[i][j]->Scale( 1.0 /(double) nEvents[i]->Integral() );
        
      }
      nEvents[i]->GetXaxis()->SetRange();
    }
  }
  
  // Used to subtract one set of 1D histograms
  // from another - does not do background sub
  // or anything like that
  
  std::vector<std::vector<TH1F*> > Subtract1D( std::vector<std::vector<TH1F*> >& base, std::vector<std::vector<TH1F*> >& subtraction, std::string uniqueID ) {
    
    // new return object
    std::vector<std::vector<TH1F*> > subtracted;
    subtracted.resize(  base.size() );
    
    for ( int i = 0; i < base.size(); ++i ) {
      subtracted[i].resize( base[i].size() );
      for ( int j = 0; j < base[i].size(); ++j ) {
        std::string tmp = uniqueID + "_subtracted_"+base[i][j]->GetName();
        subtracted[i][j] = (TH1F*) base[i][j]->Clone();
        subtracted[i][j]->SetName( tmp.c_str() );
        subtracted[i][j]->Add( subtraction[i][j], -1 );
        
      }
    }
    return subtracted;
  }
  
  // Used to subtract background from each histogram
  void SubtractBackgroundDeta( std::vector<std::vector<TH1F*> >& histograms, binSelector selector ) {
    
    //std::string etaForm = "[0]+[1]*exp(-0.5*((x-[2])/[3])**2)";
    std::string etaForm = "[0] + gausn(1)";
    std::string subForm = "[0]";
    
    
    for ( int i = 0; i < histograms.size(); ++i ) {

      for ( int j = 0; j < histograms[i].size(); ++j ) {
        
        if ( j >= 4 ) {
          continue;
        }
        
        std::cout<<"subtracting background: dEta"<<std::endl;
        std::cout<<"file: "<< i <<" pt bin: "<<j << std::endl;
        std::cout<<"function: "<< etaForm << std::endl;
        std::cout<<"over range: "<< selector.eta_fit_low_edge  << " to " << selector.eta_fit_high_edge << std::endl;
        
        std::string tmp = "fit_tmp_" + patch::to_string(i) + "_pt_" + patch::to_string(j);
        TF1* tmpFit = new TF1( tmp.c_str(), etaForm.c_str(), selector.eta_fit_low_edge, selector.eta_fit_high_edge );
        tmpFit->SetParameter( 0, histograms[i][j]->GetBinContent( histograms[i][j]->GetMinimumBin() ) );
        tmpFit->SetParameter( 1, 1 );
        tmpFit->FixParameter( 2, 0 );
        tmpFit->SetParameter( 3, 0.5 );
        
        histograms[i][j]->Fit( tmp.c_str(), "RMI" );
        std::string tmpSubName = "sub_" + tmp;
        double eta_min = histograms[i][j]->GetXaxis()->GetBinLowEdge(1);
        double eta_max = histograms[i][j]->GetXaxis()->GetBinUpEdge( selector.bindEta );
        TF1* tmpSub = new TF1( tmpSubName.c_str(), subForm.c_str(), eta_min, eta_max );
        tmpSub->SetParameter( 0, tmpFit->GetParameter(0) );;
        histograms[i][j]->Add( tmpSub, -1.0 );
        delete tmpSub;

        if ( histograms[i][j]->GetFunction(tmp.c_str() ) )
          histograms[i][j]->GetFunction(tmp.c_str())->SetBit(TF1::kNotDraw);
      }
    }
    
  }

  // same thing but with a different fit function for dphi
  void SubtractBackgroundDphi( std::vector<std::vector<TH1F*> >& histograms, binSelector selector ) {
    
    //std::string phiForm = "[0]+[1]*exp(-0.5*((x-[2])/[3])**2)+[4]*exp(-0.5*((x-[5])/[6])**2)";
    std::string phiForm = "[0] + gausn(1) + gausn(4)";
    std::string subForm = "[0]";
    
    for ( int i = 0; i < histograms.size(); ++i ) {
      for ( int j = 0; j < histograms[i].size(); ++j ) {
        
        if ( j >= 4 ) {
          continue;
        }
        
        std::cout<<"subtracting background: dPhi w/o near - far correction"<<std::endl;
        std::cout<<"file: "<<i<<" pt bin: "<<j << std::endl;
        std::cout<<"function: "<< phiForm << std::endl;
        std::cout<<"over range: "<< selector.phi_fit_low_edge  << " to " << selector.phi_fit_high_edge << std::endl;
        
        std::string tmp = "fit_tmp_" + patch::to_string(i) + "_pt_" + patch::to_string(j);
        TF1* tmpFit = new TF1( tmp.c_str(), phiForm.c_str(), selector.phi_fit_low_edge, selector.phi_fit_high_edge );
        tmpFit->SetParameter( 0, histograms[i][j]->GetBinContent( histograms[i][j]->GetMinimumBin() ) );
        tmpFit->SetParameter( 1, 1 );
        tmpFit->FixParameter( 2, 0 );
        tmpFit->SetParameter( 3, 0.5 );
        tmpFit->SetParameter( 4, 1 );
        tmpFit->FixParameter( 5, jetHadron::pi );
        tmpFit->SetParameter( 6, 0.5 );
        
        if ( i == 0 && j == 1 ) {
          tmpFit->SetParameter( 3, 0.4 );
          tmpFit->SetParameter( 6, 0.4 );
        }
        
        histograms[i][j]->Fit( tmp.c_str(), "RMI" );
        
        std::string tmpSubName = "sub_" + tmp;
        TF1* tmpSub = new TF1( tmpSubName.c_str(), subForm.c_str(), selector.phi_fit_low_edge, selector.phi_fit_high_edge );
        tmpSub->SetParameter( 0, tmpFit->GetParameter(0) );
        
        histograms[i][j]->Add( tmpSub, -1 );
        delete tmpSub;
        
        if ( histograms[i][j]->GetFunction(tmp.c_str() ) )
          histograms[i][j]->GetFunction(tmp.c_str())->SetBit(TF1::kNotDraw);

      }
    }

  }

  
  // Used to fit each histogram
  std::vector<std::vector<TF1*> > FitDeta( std::vector<std::vector<TH1F*> >& histograms, binSelector selector, std::string uniqueID ) {
    
    //std::string etaForm = "[0]+[1]*exp(-0.5*((x-[2])/[3])**2)";
    std::string etaForm = "[0] + gausn(1)";
    std::string subForm = "[0]";
    
    // return value
    std::vector<std::vector<TF1*> > fits;
    fits.resize( histograms.size() );
    
    for ( int i = 0; i < histograms.size(); ++i ) {
      fits[i].resize( histograms[i].size() );
      for ( int j = 0; j < histograms[i].size(); ++j ) {
        
        std::cout<<"fitting function: dEta"<<std::endl;
        std::cout<<"file: "<< i <<" pt bin: "<<j << std::endl;
        std::cout<<"function: "<< etaForm << std::endl;
        std::cout<<"over range: "<< selector.eta_fit_low_edge  << " to " << selector.eta_fit_high_edge << std::endl;
      
        std::string tmp = uniqueID + "fit_"; tmp += histograms[i][j]->GetName();
        fits[i][j] = new TF1( tmp.c_str(), etaForm.c_str(), selector.eta_fit_low_edge, selector.eta_fit_high_edge );
        fits[i][j]->SetParameter( 0, 0 );
        fits[i][j]->SetParameter( 1, 1 );
        fits[i][j]->FixParameter( 2, 0 );
        fits[i][j]->SetParameter( 3, 0.5 );

        
        histograms[i][j]->Fit( tmp.c_str(), "RMI", "", selector.eta_fit_low_edge, selector.eta_fit_high_edge );
        
        if ( histograms[i][j]->GetFunction(tmp.c_str() ) )
          histograms[i][j]->GetFunction(tmp.c_str())->SetBit(TF1::kNotDraw);
        
      }
    }
    
    return fits;
  }
  
  // same for dphi
  std::vector<std::vector<TF1*> > FitDphi( std::vector<std::vector<TH1F*> >& histograms, binSelector selector, std::string uniqueID ) {
    
    //std::string phiForm = "[0]+[1]*exp(-0.5*((x-[2])/[3])**2)+[4]*exp(-0.5*((x-[5])/[6])**2)";
    std::string phiForm = "[0] + gausn(1) + gausn(4)";
    std::string subForm = "[0]";
    
    // return value
    std::vector<std::vector<TF1*> > fits;
    fits.resize( histograms.size() );
    
    for ( int i = 0; i < histograms.size(); ++i ) {
      fits[i].resize(histograms[i].size() );
      
      for ( int j = 0; j < histograms[i].size(); ++j ) {
        
        std::cout<<"fitting function: dPhi w/o near - far correction"<<std::endl;
        std::cout<<"file: "<<i<<" pt bin: "<<j << std::endl;
        std::cout<<"function: "<< phiForm << std::endl;
        std::cout<<"over range: "<< selector.phi_fit_low_edge  << " to " << selector.phi_fit_high_edge << std::endl;
        
        std::string tmp = uniqueID + "fit_"; tmp += histograms[i][j]->GetName();
        fits[i][j] = new TF1( tmp.c_str(), phiForm.c_str(), selector.phi_fit_low_edge, selector.phi_fit_high_edge );
        fits[i][j]->SetParameter( 0, 0 );
        fits[i][j]->SetParameter( 1, 1 );
        fits[i][j]->FixParameter( 2, 0 );
        fits[i][j]->SetParameter( 3, 0.5 );
        fits[i][j]->SetParameter( 4, 1 );
        fits[i][j]->FixParameter( 5, jetHadron::pi );
        fits[i][j]->SetParameter( 6, 0.5 );
        
        histograms[i][j]->Fit( tmp.c_str(), "RMI", "", selector.phi_fit_low_edge, selector.phi_fit_high_edge );
        
        if ( histograms[i][j]->GetFunction(tmp.c_str() ) )
          histograms[i][j]->GetFunction(tmp.c_str())->SetBit(TF1::kNotDraw);
        
      }
    }
    return fits;
  }
  
  // fits dPhi with only a single gaussian on the
  // near side for subtracted results
  std::vector<std::vector<TF1*> > FitDphiRestricted( std::vector<std::vector<TH1F*> >& histograms, binSelector selector, std::string uniqueID ) {
    //std::string phiForm = "[0]+[1]*exp(-0.5*((x-[2])/[3])**2)+[4]*exp(-0.5*((x-[5])/[6])**2)";
    std::string phiForm = "[0] + gausn(1)";
    std::string subForm = "[0]";
    
    // return value
    std::vector<std::vector<TF1*> > fits;
    fits.resize( histograms.size() );
    
    for ( int i = 0; i < histograms.size(); ++i ) {
      fits[i].resize(histograms[i].size() );
      
      for ( int j = 0; j < histograms[i].size(); ++j ) {
        
        std::cout<<"fitting function: dPhi with near minus far correction"<<std::endl;
        std::cout<<"file: "<<i<<" pt bin: "<<j << std::endl;
        std::cout<<"function: "<< phiForm << std::endl;
        std::cout<<"over range: "<< selector.phi_corrected_fit_low_edge  << " to " << selector.phi_corrected_fit_high_edge << std::endl;

        std::string tmp = uniqueID + "fit_"; tmp += histograms[i][j]->GetName();
        fits[i][j] = new TF1( tmp.c_str(), phiForm.c_str(), selector.phi_corrected_fit_low_edge, selector.phi_corrected_fit_high_edge );
        fits[i][j]->SetParameter( 0, 0 );
        fits[i][j]->SetParameter( 1, 1 );
        fits[i][j]->FixParameter( 2, 0 );
        fits[i][j]->SetParameter( 3, 0.5 );
        
        histograms[i][j]->Fit( tmp.c_str(), "RMI", "", selector.phi_corrected_fit_low_edge, selector.phi_corrected_fit_high_edge );
        
        if ( histograms[i][j]->GetFunction(tmp.c_str() ) )
          histograms[i][j]->GetFunction(tmp.c_str())->SetBit(TF1::kNotDraw);
        
      }
    }
    return fits;
  }
  
  
  
  // Extracts the yield and errors from the fits
  // only extracts for near side so can be used
  // for both dphi and deta
  void ExtractFitVals( std::vector<std::vector<TF1*> >& fits, std::vector<std::vector<double> >& yields, std::vector<std::vector<double> >& widths, std::vector<std::vector<double> >& normError, std::vector<std::vector<double> >& widthError, binSelector selector ) {
    
    // first expand the structures
    yields.resize( fits.size() );
    widths.resize( fits.size() );
    normError.resize( fits.size() );
    widthError.resize( fits.size() );
    
    for ( int i = 0; i < fits.size(); ++i ) {
      yields[i].resize( fits[i].size() );
      widths[i].resize( fits[i].size() );
      normError[i].resize( fits[i].size() );
      widthError[i].resize( fits[i].size() );
      
      for ( int j = 0; j < fits[i].size(); ++j ) {
        
        //yields[i][j] = fits[i][j]->GetParameter(1)*sqrt(2*pi)*fabs(fits[i][j]->GetParameter(3))/selector.GetPtBinWidth( j );
        yields[i][j] = fits[i][j]->GetParameter(1);
        widths[i][j] = fabs(fits[i][j]->GetParameter(3));
        normError[i][j] = fits[i][j]->GetParError( 1 );
        widthError[i][j] = fits[i][j]->GetParError( 3 );
        
      }
    }
  }
  
  // Used to get the integrals of the
  // histograms, and errors on the integrals
  void ExtractIntegraldPhi( std::vector<std::vector<TH1F*> >& histograms, std::vector<std::vector<double> >& integrals, std::vector<std::vector<double> >& errors, binSelector selector ) {
    
    // resize the output containers
    integrals.resize( histograms.size() );
    errors.resize( histograms.size() );
    
    for ( int i = 0; i < histograms.size(); ++i ) {
      
      //resize inner output containers
      integrals[i].resize( histograms[i].size() );
      errors[i].resize( histograms[i].size() );
      
      for ( int j = 0; j < histograms[i].size(); ++j ) {
        integrals[i][j] = histograms[i][j]->IntegralAndError( histograms[i][j]->GetXaxis()->FindBin(selector.phi_projection_integral_range_low), histograms[i][j]->GetXaxis()->FindBin(selector.phi_projection_integral_range_high), errors[i][j], "width");
      }
      
    }
  }
  
  void ExtractIntegraldEta( std::vector<std::vector<TH1F*> >& histograms, std::vector<std::vector<double> >& integrals, std::vector<std::vector<double> >& errors, binSelector selector ) {
    
    // resize the output containers
    integrals.resize( histograms.size() );
    errors.resize( histograms.size() );
    
    for ( int i = 0; i < histograms.size(); ++i ) {
      
      //resize inner output containers
      integrals[i].resize( histograms[i].size() );
      errors[i].resize( histograms[i].size() );
      
      for ( int j = 0; j < histograms[i].size(); ++j ) {
        integrals[i][j] = histograms[i][j]->IntegralAndError( histograms[i][j]->GetXaxis()->FindBin(selector.eta_projection_integral_range_low), histograms[i][j]->GetXaxis()->FindBin(selector.eta_projection_integral_range_high), errors[i][j], "width");
      }
      
    }
  }

  
  // testing function to fix histogram
  // bins not being drawn if the content is small
  void FixTheDamnBins( std::vector<std::vector<TH1F*> >& histograms ) {
    
    for ( int i = 0; i < histograms.size(); ++i ) {
      for ( int j = 0; j < histograms[i].size(); ++j ) {
        for ( int k = 1; k < histograms[i][j]->GetXaxis()->GetNbins(); ++k ) {
          
          if ( fabs(histograms[i][j]->GetBinContent(k)) == 0 && fabs( histograms[i][j]->GetBinError(k))== 0 ) {
            histograms[i][j]->SetBinContent(k, 0.0001 );
            histograms[i][j]->SetBinError( k, 0.0001 );
          }
          
        }
      }
    }
    
  }
  
  void FixTheDamnBins( std::vector<TH1F*>& histograms ) {
    
    for ( int i = 0; i < histograms.size(); ++i ) {
      for ( int k = 1; k < histograms[i]->GetXaxis()->GetNbins(); ++k ) {
        if ( fabs(histograms[i]->GetBinContent(k)) == 0 && fabs( histograms[i]->GetBinError(k))== 0 ) {
          histograms[i]->SetBinContent(k, 0.0001 );
          histograms[i]->SetBinError( k, 0.0001 );
        }
      }
    }
  }
  
  std::vector<TGraphErrors*> MakeGraphs( std::vector<std::vector<double> >& x, std::vector<std::vector<double> >& y, std::vector<std::vector<double> >& x_err, std::vector<std::vector<double> >& y_err, int ptBinLow, int ptBinHigh, binSelector selector, std::vector<std::string> analysisName, std::string uniqueID ) {
    
    // making some tgraphs to plot and/or save
    std::vector<TGraphErrors*> returnGraph;
    returnGraph.resize( y.size() );
    
    for ( int i = 0; i < y.size(); ++i ) {
      int ptBins = ptBinHigh - ptBinLow + 1;
      
      double x_[ptBins];
      double y_[ptBins];
      double x_err_[ptBins];
      double y_err_[ptBins];
      
      for ( int j = ptBinLow; j <= ptBinHigh; ++j ) {
        x_[j-ptBinLow] = x[i][j];
        y_[j-ptBinLow] = y[i][j] / selector.GetPtBinWidth(j);
        x_err_[j-ptBinLow] = x_err[i][j];
        y_err_[j-ptBinLow] = y_err[i][j];
 
      }
      
      std::string tmp = uniqueID + "_graph_" + analysisName[i];
 
      returnGraph[i] = new TGraphErrors( ptBins, x_, y_, x_err_, y_err_ );
      returnGraph[i]->SetName( tmp.c_str() );

    }
    
    return returnGraph;
  }
  
  
  // ***************************************
  // these are used for building uncertainty
  // bands for the pp data
  // ****************************************
  // Used to create systematic uncertainty bands
  // from varying tower energy scale / TPC tracking efficiency
  std::vector<TH1F*> BuildSystematicHistogram( std::vector<TH1F*>& upper, std::vector<TH1F*>& lower, binSelector selector, std::string uniqueID ) {
    
    std::vector<TH1F*> histograms;
    histograms.resize( upper.size() );
    
    for ( int i = 0; i < upper.size(); ++i ) {
    
      std::string tmp = uniqueID + "_systematic_pt_"+ patch::to_string(i);
      
      histograms[i] = new TH1F( tmp.c_str(), selector.ptBinString[i].c_str(), upper[i]->GetXaxis()->GetNbins(), upper[i]->GetXaxis()->GetBinLowEdge(1), upper[i]->GetXaxis()->GetBinUpEdge(upper[i]->GetXaxis()->GetNbins()) );
      
      for ( int j = 1; j <= upper[i]->GetXaxis()->GetNbins(); ++j ) {
        double binContent = fabs( upper[i]->GetBinContent( j ) + lower[i]->GetBinContent( j ) ) / 2.0;
        double binWidth = fabs( upper[i]->GetBinContent( j ) - lower[i]->GetBinContent( j ) );
        
        histograms[i]->SetBinContent( j, binContent );
        histograms[i]->SetBinError( j, binWidth );
        
      }
      
    }
    
    return histograms;
  }
  
  // Used to add systematic errors in quadrature
  // For when we plot both tower energy & tracking efficiency
  std::vector<TH1F*> AddInQuadrature( std::vector<TH1F*> hist1, std::vector<TH1F*> hist2, binSelector selector, std::string uniqueID ) {
    
    std::vector<TH1F*> histograms;
    histograms.resize( hist1.size() );
    
    for ( int i = 0; i < hist1.size(); ++i ) {
      
      std::string tmp = uniqueID + "_sys_quad_pt_"+ patch::to_string(i);
      
      histograms[i] = new TH1F( tmp.c_str(), selector.ptBinString[i].c_str(), hist1[i]->GetXaxis()->GetNbins(), hist1[i]->GetXaxis()->GetBinLowEdge(1), hist1[i]->GetXaxis()->GetBinUpEdge(hist1[i]->GetXaxis()->GetNbins()) );
      
      for ( int j = 1; j <= hist1[i]->GetXaxis()->GetNbins(); ++j ) {
        
        double binContent = fabs( hist1[i]->GetBinContent( j ) + hist2[i]->GetBinContent( j ) ) / 2.0;
        double binWidth = sqrt(fabs( hist1[i]->GetBinError( j )*hist1[i]->GetBinError( j ) + hist2[i]->GetBinError( j )*hist2[i]->GetBinError( j ) ) );
        
        histograms[i]->SetBinContent( j, binContent );
        histograms[i]->SetBinError( j, binWidth );
        
      }
    }
    
    return histograms;
  }
  
  // and using only numbers
  std::vector<double> AddInQuadrature( std::vector<double> upper, std::vector<double> lower ) {
    std::vector<double> values;
    values.resize( upper.size() );
    
    for ( int i = 0; i < values.size(); ++i ) {
      
      values[i] = sqrt ( upper[i]*upper[i] + lower[i]*lower[i] );
      
    }
    
    return values;
  }
  
  
  // used to make 5% errors on yields due to tracking
  std::vector<std::vector<TH1F*> > BuildYieldError( std::vector<std::vector<TH1F*> > histograms, binSelector selector, std::vector<std::string> analysisName, std::string uniqueID  ) {
    
    std::vector<std::vector<TH1F*> > returnHist;
    returnHist.resize( histograms.size() );
    
    for ( int i = 0; i < histograms.size(); ++i ) {
      
      returnHist[i].resize( histograms[i].size() );
      
      for ( int j = 0; j < histograms[i].size(); ++j ) {
      
        std::string tmp = uniqueID + "_yield_sys_err_" + analysisName[i] + "_pt_"+ patch::to_string(j);
      
        returnHist[i][j] = new TH1F( tmp.c_str(), selector.ptBinString[j].c_str(), histograms[i][j]->GetXaxis()->GetNbins(), histograms[i][j]->GetXaxis()->GetBinLowEdge(1), histograms[i][j]->GetXaxis()->GetBinUpEdge(histograms[i][j]->GetXaxis()->GetNbins()) );
      
        for ( int k = 1; k <= histograms[i][j]->GetXaxis()->GetNbins(); ++k ) {
        
          double binContent = histograms[i][j]->GetBinContent( k );
          double binWidth = histograms[i][j]->GetBinContent( k ) * 0.05;
        
          returnHist[i][j]->SetBinContent( k, binContent );
          returnHist[i][j]->SetBinError( k, binWidth );
        }
        
      }
      
    }
    
    return returnHist;
  }
  
  // used to make full 5% errors on yields
  std::vector<std::vector<double> > BuildYieldError ( std::vector<std::vector<double> > &yields, binSelector selector ) {
    std::vector<std::vector<double> > errors;
    errors.resize( yields.size() );
    
    for ( int i = 0; i < yields.size(); ++i ) {
      
      errors[i].resize( yields[i].size() );
      
      for ( int j = 0; j < yields[i].size(); ++j ) {
        
        errors[i][j] = 0.05 * yields[i][j] / selector.GetPtBinWidth(j);
        
      }
      
    }
    
    return errors;
  }
  
  
  // testing function to reset the bin contents to those of the histogram
  void ResetSysBinContent( std::vector<TH1F*>& errors, std::vector<TH1F*>& histograms, binSelector selector ) {
    
    if ( histograms.size() != errors.size() ) {
      __ERR("warning: mismatched bin sizes")
      return;
    }
    
    if ( histograms[0]->GetXaxis()->GetNbins() != errors[0]->GetXaxis()->GetNbins() ) {
      __ERR("warning: bin mismatch between errors and histograms")
      return;
    }
    
    for ( int i = 0; i < errors.size(); ++i ) {
      
      for ( int j = 1; j <= errors[i]->GetXaxis()->GetNbins(); ++j ) {
        errors[i]->SetBinContent( j, histograms[i]->GetBinContent( j ) );
        
      }
      
    }
    
    
  }
  
  // and used to scale errors by the pt bin width
  void ScaleErrors( std::vector<std::vector<double> > errors, binSelector selector ) {
  
    for ( int i = 0; i < errors.size(); ++i ) {
      for ( int j = 0; j < errors[i].size(); ++j ) {
        errors[i][j] *= 1.0/selector.GetPtBinWidth(j);
      }
    }
  }
  
  // used to extract only the yields, dont need the errors
  std::vector<std::vector<double> > OnlyYieldsEta( std::vector<std::vector<TH1F*> >& histograms, binSelector selector ) {
    
    std::vector<std::vector<double> > yields;
    yields.resize ( histograms.size() );
    
    for ( int i = 0; i < histograms.size(); ++i ) {
      
      yields[i].resize( histograms[i].size() );
      
      for ( int j = 0; j < histograms[i].size(); ++j ) {
        
        yields[i][j] = histograms[i][j]->Integral( histograms[i][j]->GetXaxis()->FindBin(selector.eta_projection_integral_range_low), histograms[i][j]->GetXaxis()->FindBin(selector.eta_projection_integral_range_high), "width");
        
        
      }
      
    }
    return yields;
  }
  
  // used to extract only the yields, dont need the errors
  std::vector<std::vector<double> > OnlyYieldsPhi( std::vector<std::vector<TH1F*> >& histograms, binSelector selector ) {
    
    std::vector<std::vector<double> > yields;
    yields.resize ( histograms.size() );
    
    for ( int i = 0; i < histograms.size(); ++i ) {
      
      yields[i].resize( histograms[i].size() );
      
      for ( int j = 0; j < histograms[i].size(); ++j ) {
        
        yields[i][j] = histograms[i][j]->Integral( histograms[i][j]->GetXaxis()->FindBin(selector.phi_projection_integral_range_low), histograms[i][j]->GetXaxis()->FindBin(selector.phi_projection_integral_range_high), "width");
        
        
      }
      
    }
    return yields;
  }
  
  // used to get the difference between two sets of values... specific use
  std::vector<double> GetDifference( std::vector<std::vector<double> >& yields ) {
    
    std::vector<double> diff;
    
    for ( int i = 0; i < yields[0].size(); ++i ) {
      diff.push_back( fabs( yields[0][i] - yields[1][i] ) );
    }
    return diff;
  }
  
  
  // *****************************
  // HISTOGRAM PRINTING AND SAVING
  // *****************************
  
  // Used internally to find good ranges for histograms
  void FindGood1DUserRange( std::vector<TH1F*> histograms, double& max, double& min ) {
    
    double tmpMin = 0;
    double tmpMax = 0;
    
    for ( int i = 0; i < histograms.size(); ++i ) {
      if ( i == 0 ) {
        tmpMax = histograms[i]->GetMaximum();
        tmpMin = histograms[i]->GetMinimum();
      }
      else {
        if ( histograms[i]->GetMaximum() > tmpMax ) { tmpMax = histograms[i]->GetMaximum(); }
        if ( histograms[i]->GetMinimum() < tmpMin ) { tmpMin = histograms[i]->GetMinimum(); }
      }
    }
    max = 1.2*tmpMax;
    min = 0.8*fabs(tmpMin);
    if ( min > -0.1 )
      min = -1.0;
    if ( max < 1.0 )
      max = 1.0;
    if ( max > 4.0 )
      max = 4.0;
         
  }
  
  void FindGood1DUserRange( std::vector<TH1F*> histograms, double& max, double& min, double xMax, double xMin ) {
    
    double tmpMin = 0;
    double tmpMax = 0;
    
    for ( int i = 0; i < histograms.size(); ++i ) {
      histograms[i]->GetXaxis()->SetRangeUser( xMin, xMax );
      
      if ( i == 0 ) {
        tmpMax = histograms[i]->GetMaximum();
        tmpMin = histograms[i]->GetMinimum();
      }
      else {
        if ( histograms[i]->GetMaximum() > tmpMax ) { tmpMax = histograms[i]->GetMaximum(); }
        if ( histograms[i]->GetMinimum() < tmpMin ) { tmpMin = histograms[i]->GetMinimum(); }
      }
      histograms[i]->GetXaxis()->SetRange();
    }
    max = 1.2*tmpMax;
    min = 0.8*fabs(tmpMin);
    if ( min > -0.1 )
      min = -1.0;
    if ( max < 1.0 )
      max = 1.0;
    if ( max > 4.0 )
      max = 4.0;
    
  }

  
  // Used to print out 2D plots ( correlations, mixed events )
  void Print2DHistograms( std::vector<TH2F*>& histograms, std::string outputDir, std::string analysisName, binSelector selector ) {
    
    // First, make the output directory if it doesnt exist
    boost::filesystem::path dir( outputDir.c_str() );
    boost::filesystem::create_directories( dir );
    
    for ( int i = 0; i < histograms.size(); ++i ) {
      
      histograms[i]->GetXaxis()->SetTitle("#Delta#eta");
      histograms[i]->GetXaxis()->SetTitleSize( 0.06 );
      histograms[i]->GetXaxis()->SetTitleOffset( 1.35 );
      histograms[i]->GetXaxis()->CenterTitle( true );
      //histograms[i]->GetXaxis()->SetRange(5,18);
      histograms[i]->GetYaxis()->SetTitle("#Delta#phi");
      histograms[i]->GetYaxis()->SetTitleSize( 0.06 );
      histograms[i]->GetYaxis()->SetTitleOffset( 1.35 );
      histograms[i]->GetYaxis()->CenterTitle( true );
      histograms[i]->GetZaxis()->SetTitle("counts");
      histograms[i]->GetZaxis()->SetTitleSize( 0.05 );
      //histograms[i]->GetZaxis()->SetTitleOffset( 1.35 );
      histograms[i]->GetZaxis()->CenterTitle( true );
      histograms[i]->SetTitle( selector.ptBinString[i].c_str() );
      
      std::string tmp = outputDir + "/" + analysisName + "_" + patch::to_string(i) + ".pdf";
      
      TCanvas c1;
      c1.SetLeftMargin(0.15);
      c1.SetBottomMargin(0.2);
      histograms[i]->Draw("surf1");
      c1.SaveAs( tmp.c_str() );
      tmp = outputDir + "/" + analysisName + "_" + patch::to_string(i) + ".C";
      c1.SaveAs( tmp.c_str() );
    }
    
  }
  
  void Print2DHistogramsMixing( std::vector<TH2F*>& histograms, std::string outputDir, std::string analysisName, binSelector selector ) {
    
    // First, make the output directory if it doesnt exist
    boost::filesystem::path dir( outputDir.c_str() );
    boost::filesystem::create_directories( dir );
    
    for ( int i = 0; i < histograms.size(); ++i ) {
      
      histograms[i]->GetXaxis()->SetTitle("#Delta#eta");
      histograms[i]->GetXaxis()->SetTitleSize( 0.06 );
      histograms[i]->GetXaxis()->SetTitleOffset( 1.35 );
      histograms[i]->GetXaxis()->CenterTitle( true );
      histograms[i]->GetYaxis()->SetTitle("#Delta#phi");
      histograms[i]->GetYaxis()->SetTitleSize( 0.06 );
      histograms[i]->GetYaxis()->SetTitleOffset( 1.35 );
      histograms[i]->GetYaxis()->CenterTitle( true );
      histograms[i]->GetZaxis()->SetTitle("counts");
      histograms[i]->GetZaxis()->SetTitleSize( 0.05 );
      //histograms[i]->GetZaxis()->SetTitleOffset( 1.35 );
      histograms[i]->GetZaxis()->CenterTitle( true );
      histograms[i]->SetTitle( selector.ptBinStringMix[i].c_str() );
      
      std::string tmp = outputDir + "/" + analysisName + "_" + patch::to_string(i) + ".pdf";
      
      TCanvas c1;
      c1.SetLeftMargin(0.15);
      c1.SetBottomMargin(0.2);
      histograms[i]->Draw("surf1");
      c1.SaveAs( tmp.c_str() );
      tmp = outputDir + "/" + analysisName + "_" + patch::to_string(i) + ".pdf";
      c1.SaveAs( tmp.c_str() );
    }
    
  }
  
  // Used to print out 2D plots ( correlations, mixed events )
  // However, this one restricts the eta range shown to what
  // is set in selector
  void Print2DHistogramsEtaRestricted( std::vector<TH2F*>& histograms, std::string outputDir, std::string analysisName, binSelector selector ) {
    
    // First, make the output directory if it doesnt exist
    boost::filesystem::path dir( outputDir.c_str() );
    boost::filesystem::create_directories( dir );
    
    for ( int i = 0; i < histograms.size(); ++i ) {
      
      histograms[i]->GetXaxis()->SetTitle("#Delta#eta");
      histograms[i]->GetXaxis()->SetTitleSize( 0.06 );
      histograms[i]->GetXaxis()->SetTitleOffset( 1.35 );
      histograms[i]->GetXaxis()->CenterTitle( true );
      histograms[i]->GetXaxis()->SetRange(5, 18 );
      histograms[i]->GetYaxis()->SetTitle("#Delta#phi");
      histograms[i]->GetYaxis()->SetTitleSize( 0.06 );
      histograms[i]->GetYaxis()->SetTitleOffset( 1.35 );
      histograms[i]->GetYaxis()->CenterTitle( true );
      histograms[i]->GetZaxis()->SetTitle("counts");
      histograms[i]->GetZaxis()->SetTitleSize( 0.05 );
      //histograms[i]->GetZaxis()->SetTitleOffset( 1.35 );
      histograms[i]->GetZaxis()->CenterTitle( true );
      histograms[i]->SetTitle( selector.ptBinString[i].c_str() );
      std::string tmp = outputDir + "/" + analysisName + "_" + patch::to_string(i) + ".pdf";
      TCanvas c1;
      c1.SetLeftMargin(0.15);
      c1.SetBottomMargin(0.2);
    
      histograms[i]->Draw("surf1");
      c1.SaveAs( tmp.c_str() );
      tmp = outputDir + "/" + analysisName + "_" + patch::to_string(i) + ".C";
      c1.SaveAs( tmp.c_str() );
    }
    
  }
  
  // Print out individual dPhi histograms
  void Print1DHistogramsDphi( std::vector<TH1F*>& histograms, std::string outputDir, std::string analysisName, binSelector selector ) {
  
    // First, make the output directory if it doesnt exist
    boost::filesystem::path dir( outputDir.c_str() );
    boost::filesystem::create_directories( dir );
    
    for ( int i = 0; i < histograms.size(); ++i ) {
      
      double min, max;
      std::vector<TH1F*> tmpvec;
      tmpvec.push_back( histograms[i] );
      FindGood1DUserRange( tmpvec, max, min );
      
      histograms[i]->GetXaxis()->SetTitle("#Delta#phi");
      histograms[i]->GetXaxis()->SetTitleSize( 0.06 );
      histograms[i]->GetYaxis()->SetTitle( "1/N_{Dijet}dN/d#phi");
      histograms[i]->GetYaxis()->SetTitleSize( 0.04 );
      histograms[i]->SetTitle( selector.ptBinString[i].c_str() );
      histograms[i]->GetYaxis()->SetRangeUser(min , max );
      
      std::string tmp = outputDir + "/" + analysisName + "_" + patch::to_string(i) + ".pdf";
      
      TCanvas c1;
      c1.SetLeftMargin(0.15);
      c1.SetBottomMargin(0.2);
      histograms[i]->Draw();
      c1.SaveAs( tmp.c_str() );
      tmp = outputDir + "/" + analysisName + "_" + patch::to_string(i) + ".C";
      c1.SaveAs( tmp.c_str() );
    }
    
  }
  
  // Print out dPhi histograms, each pt bin
  // Seperately, but each file overlaid
  void Print1DHistogramsOverlayedDphi( std::vector<std::vector<TH1F*> >& histograms, std::string outputDir, std::vector<std::string> analysisName, binSelector selector ) {
    
    // First, make the output directory if it doesnt exist
    boost::filesystem::path dir( outputDir.c_str() );
    boost::filesystem::create_directories( dir );
    
    TCanvas c1;
    c1.SetLeftMargin(0.15);
    c1.SetBottomMargin(0.2);
    for ( int i = 0; i < histograms[0].size(); ++i ) {
      
      double min, max;
      std::vector<TH1F*> tmpvec;
      for ( int j = 0; j < histograms.size(); ++j ) {
        tmpvec.push_back( histograms[j][i] );
      }
      FindGood1DUserRange( tmpvec, max, min );
      
      TLegend* leg = new TLegend(0.6, 0.6, .88, .88);
      leg->SetTextSize(0.04);
      
      for ( int j = 0; j < histograms.size(); ++j ) {
        
      
        histograms[j][i]->GetXaxis()->SetTitle("#Delta#phi");
        histograms[j][i]->GetXaxis()->SetTitleSize( 0.06 );
        histograms[j][i]->GetYaxis()->SetTitle( "1/N_{Dijet}dN/d#phi");
        histograms[j][i]->GetYaxis()->SetTitleSize( 0.04 );
        histograms[j][i]->SetTitle( selector.ptBinString[i].c_str() );
        histograms[j][i]->SetLineColor( j+1 );
        histograms[j][i]->SetMarkerStyle( j+20 );
        histograms[j][i]->SetMarkerColor( j+1 );
        histograms[j][i]->SetMarkerSize( 2 );
        histograms[j][i]->GetYaxis()->SetRangeUser( min, max);
      
        if ( j == 0 ) {
        histograms[j][i]->Draw();
        }
        else {
          histograms[j][i]->Draw("same");
        }
        
        // add to legend
        if ( histograms.size() <= 2 )
          leg->AddEntry( histograms[j][i], selector.analysisStrings[j].c_str(), "lep" );
        else
          leg->AddEntry( histograms[j][i], analysisName[j].c_str(), "lep" );
        
      }
      leg->Draw();
      std::string tmp = outputDir + "/" + analysisName[0] + "_" + patch::to_string(i) + ".pdf";
      c1.SaveAs( tmp.c_str() );
      tmp = outputDir + "/" + analysisName[0] + "_" + patch::to_string(i) + ".C";
      c1.SaveAs( tmp.c_str() );
    }
    
  }
  
  // Print out dPhi histograms, each pt bin
  // Seperately, but each file overlaid
  void Print1DHistogramsOverlayedDphiWFit( std::vector<std::vector<TH1F*> >& histograms, std::vector<std::vector<TF1*> >& fits, std::string outputDir, std::vector<std::string> analysisName, binSelector selector ) {
    
    // First, make the output directory if it doesnt exist
    boost::filesystem::path dir( outputDir.c_str() );
    boost::filesystem::create_directories( dir );
    
    TCanvas c1;
    c1.SetLeftMargin(0.15);
    c1.SetBottomMargin(0.2);
    for ( int i = 0; i < histograms[0].size(); ++i ) {
      
      double min, max;
      std::vector<TH1F*> tmpvec;
      for ( int j = 0; j < histograms.size(); ++j ) {
        tmpvec.push_back( histograms[j][i] );
      }
      FindGood1DUserRange( tmpvec, max, min );
      
      TLegend* leg = new TLegend(0.6, 0.6, .88, .88);
      leg->SetTextSize(0.04);
      
      for ( int j = 0; j < histograms.size(); ++j ) {
        
        
        histograms[j][i]->GetXaxis()->SetTitle("#Delta#phi");
        histograms[j][i]->GetXaxis()->SetTitleSize( 0.06 );
        histograms[j][i]->GetYaxis()->SetTitle( "1/N_{Dijet}dN/d#phi");
        histograms[j][i]->GetYaxis()->SetTitleSize( 0.04 );
        histograms[j][i]->SetTitle( selector.ptBinString[i].c_str() );
        histograms[j][i]->SetLineColor( j+1 );
        histograms[j][i]->SetMarkerStyle( j+20 );
        histograms[j][i]->SetMarkerColor( j+1 );
        histograms[j][i]->SetMarkerSize( 2 );
        histograms[j][i]->GetYaxis()->SetRangeUser( min, max);
        
        // try to set a fit function if it exists
        if ( histograms[j][i]->FindObject( fits[j][i]->GetName() ) ) {
          TF1* tmp = (TF1*) histograms[j][i]->FindObject( fits[j][i]->GetName() );
          tmp->SetLineColor(j+1);
        }
        
        if ( j == 0 ) {
          histograms[j][i]->Draw();
        }
        else {
          histograms[j][i]->Draw("same");
        }
        
        // add to legend
        if ( histograms.size() <= 2 )
          leg->AddEntry( histograms[j][i], selector.analysisStrings[j].c_str(), "lep" );
        else
          leg->AddEntry( histograms[j][i], analysisName[j].c_str(), "lep" );
        
      }
      leg->Draw();
      std::string tmp = outputDir + "/" + analysisName[0] + "_" + patch::to_string(i) + ".pdf";
      c1.SaveAs( tmp.c_str() );
      tmp = outputDir + "/" + analysisName[0] + "_" + patch::to_string(i) + ".C";
      c1.SaveAs( tmp.c_str() );
    }
    
  }
  
  // Print out dPhi histograms, each pt bin
  // Seperately, but each file overlaid
  // But restricts the range to the near side only
  void Print1DHistogramsOverlayedDphiWFitRestricted( std::vector<std::vector<TH1F*> >& histograms, std::vector<std::vector<TF1*> >& fits, std::string outputDir, std::vector<std::string> analysisName, binSelector selector ) {
    
    // First, make the output directory if it doesnt exist
    boost::filesystem::path dir( outputDir.c_str() );
    boost::filesystem::create_directories( dir );
    
    TCanvas c1;
    c1.SetLeftMargin(0.15);
    c1.SetBottomMargin(0.2);
    for ( int i = 0; i < histograms[0].size(); ++i ) {
      
      double min, max;
      std::vector<TH1F*> tmpvec;
      for ( int j = 0; j < histograms.size(); ++j ) {
        tmpvec.push_back( histograms[j][i] );
      }
      FindGood1DUserRange( tmpvec, max, min );
      
      TLegend* leg = new TLegend(0.6, 0.6, .88, .88);
      leg->SetTextSize(0.04);
      
      for ( int j = 0; j < histograms.size(); ++j ) {
        
        
        histograms[j][i]->GetXaxis()->SetTitle("#Delta#phi");
        histograms[j][i]->GetXaxis()->SetTitleSize( 0.06 );
        histograms[j][i]->GetYaxis()->SetTitle( "1/N_{Dijet}dN/d#phi");
        histograms[j][i]->GetYaxis()->SetTitleSize( 0.04 );
        histograms[j][i]->SetTitle( selector.ptBinString[i].c_str() );
        histograms[j][i]->SetLineColor( j+1 );
        histograms[j][i]->SetMarkerStyle( j+20 );
        histograms[j][i]->SetMarkerColor( j+1 );
        histograms[j][i]->SetMarkerSize( 2 );
        histograms[j][i]->GetYaxis()->SetRangeUser( min, max);
        histograms[j][i]->GetXaxis()->SetRangeUser( -pi/2.0, pi/2.0 );
        
        // try to set a fit function if it exists
        if ( histograms[j][i]->FindObject( fits[j][i]->GetName() ) ) {
          TF1* tmp = (TF1*) histograms[j][i]->FindObject( fits[j][i]->GetName() );
          tmp->SetLineColor(j+1);
        }
        
        if ( j == 0 ) {
          histograms[j][i]->Draw();
        }
        else {
          histograms[j][i]->Draw("same");
        }
        
        // add to legend
        if ( histograms.size() <= 2 )
          leg->AddEntry( histograms[j][i], selector.analysisStrings[j].c_str(), "lep" );
        else
          leg->AddEntry( histograms[j][i], analysisName[j].c_str(), "lep" );
        
      }
      leg->Draw();
      std::string tmp = outputDir + "/" + analysisName[0] + "_" + patch::to_string(i) + ".pdf";
      c1.SaveAs( tmp.c_str() );
      tmp = outputDir + "/" + analysisName[0] + "_" + patch::to_string(i) + ".C";
      c1.SaveAs( tmp.c_str() );
    }
    
  }

  
  // Print out individual dEta histograms
  void Print1DHistogramsDeta( std::vector<TH1F*>& histograms, std::string outputDir, std::string analysisName, binSelector selector ) {
    
    // First, make the output directory if it doesnt exist
    boost::filesystem::path dir( outputDir.c_str() );
    boost::filesystem::create_directories( dir );
    
    for ( int i = 0; i < histograms.size(); ++i ) {
      
      double min, max;
      std::vector<TH1F*> tmpvec;
      tmpvec.push_back( histograms[i] );
      FindGood1DUserRange( tmpvec, max, min );
      
      histograms[i]->GetXaxis()->SetTitle("#Delta#eta");
      histograms[i]->GetXaxis()->SetTitleSize( 0.06 );
      histograms[i]->GetYaxis()->SetTitle( "1/N_{Dijet}dN/d#eta");
      histograms[i]->GetYaxis()->SetTitleSize( 0.04 );
      histograms[i]->SetTitle( selector.ptBinString[i].c_str() );
      histograms[i]->GetYaxis()->SetRangeUser( min, max );
      
      std::string tmp = outputDir + "/" + analysisName + "_" + patch::to_string(i) + ".pdf";
      
      TCanvas c1;
      c1.SetLeftMargin(0.15);
      c1.SetBottomMargin(0.2);
      histograms[i]->Draw();
      c1.SaveAs( tmp.c_str() );
      tmp = outputDir + "/" + analysisName + "_" + patch::to_string(i) + ".C";
      c1.SaveAs( tmp.c_str() );
    }

    
  }
  
  // Print out dPhi histograms, each pt bin
  // Seperately, but each file overlaid
  void Print1DHistogramsOverlayedDeta( std::vector<std::vector<TH1F*> >& histograms, std::string outputDir, std::vector<std::string> analysisName, binSelector selector ) {
    
    // First, make the output directory if it doesnt exist
    boost::filesystem::path dir( outputDir.c_str() );
    boost::filesystem::create_directories( dir );
    
    TCanvas c1;
    c1.SetLeftMargin(0.2);
    c1.SetBottomMargin(0.2);
    for ( int i = 0; i < histograms[0].size(); ++i ) {
      
      double min, max;
      std::vector<TH1F*> tmpvec;
      for ( int j = 0; j < histograms.size(); ++j ) {
        tmpvec.push_back( histograms[j][i] );
      }
      FindGood1DUserRange( tmpvec, max, min );
      
      TLegend* leg = new TLegend(0.6, 0.6, .88, .88);
      leg->SetTextSize(0.04);
      
      for ( int j = 0; j < histograms.size(); ++j ) {
        
        histograms[j][i]->GetXaxis()->SetTitle("#Delta#eta");
        histograms[j][i]->GetXaxis()->SetTitleSize( 0.06 );
        histograms[j][i]->GetYaxis()->SetTitle( "1/N_{Dijet}dN/d#eta");
        histograms[j][i]->GetYaxis()->SetTitleSize( 0.04 );
        histograms[j][i]->SetTitle( selector.ptBinString[i].c_str() );
        histograms[j][i]->SetLineColor( j+1 );
        histograms[j][i]->SetMarkerStyle( j+20 );
        histograms[j][i]->SetMarkerColor( j+1 );
        histograms[j][i]->SetMarkerSize( 2 );
        histograms[j][i]->GetYaxis()->SetRangeUser( min, max );

        
        if ( j == 0 ) {
          histograms[j][i]->Draw();
        }
        else {
          histograms[j][i]->Draw("same");
        }
        
        // add to legend
        if ( histograms.size() <= 2 )
          leg->AddEntry( histograms[j][i], selector.analysisStrings[j].c_str(), "lep" );
        else
          leg->AddEntry( histograms[j][i], analysisName[j].c_str(), "lep" );
        
      }
      leg->Draw();
      std::string tmp = outputDir + "/" + analysisName[0] + "_" + patch::to_string(i) + ".pdf";
      c1.SaveAs( tmp.c_str() );
      tmp = outputDir + "/" + analysisName[0] + "_" + patch::to_string(i) + ".C";
      c1.SaveAs( tmp.c_str() );
    }

    
  }
  
  // Print out dPhi histograms, each pt bin
  // Seperately, but each file overlaid
  void Print1DHistogramsOverlayedDetaWFit( std::vector<std::vector<TH1F*> >& histograms, std::vector<std::vector<TF1*> >& fits, std::string outputDir, std::vector<std::string> analysisName, binSelector selector ) {
    
    // First, make the output directory if it doesnt exist
    boost::filesystem::path dir( outputDir.c_str() );
    boost::filesystem::create_directories( dir );
    
    TCanvas c1;
    c1.SetLeftMargin(0.15);
    c1.SetBottomMargin(0.2);
    for ( int i = 0; i < histograms[0].size(); ++i ) {
      
      double min, max;
      std::vector<TH1F*> tmpvec;
      for ( int j = 0; j < histograms.size(); ++j ) {
        tmpvec.push_back( histograms[j][i] );
      }
      
      FindGood1DUserRange( tmpvec, max, min );
      
      TLegend* leg = new TLegend(0.6, 0.6, .88, .88);
      leg->SetTextSize(0.04);
      
      for ( int j = 0; j < histograms.size(); ++j ) {
        
        histograms[j][i]->GetXaxis()->SetTitle("#Delta#eta");
        histograms[j][i]->GetXaxis()->SetTitleSize( 0.06 );
        histograms[j][i]->GetYaxis()->SetTitle( "1/N_{Dijet}dN/d#eta");
        histograms[j][i]->GetYaxis()->SetTitleSize( 0.04 );
        histograms[j][i]->SetTitle( selector.ptBinString[i].c_str() );
        histograms[j][i]->SetLineColor( j+1 );
        histograms[j][i]->SetMarkerStyle( j+20 );
        histograms[j][i]->SetMarkerColor( j+1 );
        histograms[j][i]->SetMarkerSize( 2 );
        histograms[j][i]->GetYaxis()->SetRangeUser( min, max );
        
        // try to set a fit function if it exists
        if ( histograms[j][i]->FindObject( fits[j][i]->GetName() ) ) {
          TF1* tmp = (TF1*) histograms[j][i]->FindObject( fits[j][i]->GetName() );
          tmp->SetLineColor(j+1);
        }
        
        if ( j == 0 ) {
          histograms[j][i]->Draw();
        }
        else {
          histograms[j][i]->Draw("same");
        }
        
        // add to legend
        if ( histograms.size() <= 2 )
          leg->AddEntry( histograms[j][i], selector.analysisStrings[j].c_str(), "lep" );
        else
          leg->AddEntry( histograms[j][i], analysisName[j].c_str(), "lep" );
        
      }
      leg->Draw();
      std::string tmp = outputDir + "/" + analysisName[0] + "_" + patch::to_string(i) + ".pdf";
      c1.SaveAs( tmp.c_str() );
      tmp = outputDir + "/" + analysisName[0] + "_" + patch::to_string(i) + ".C";
      c1.SaveAs( tmp.c_str() );
    }
    
    
  }
  
  // Print out dPhi histograms, each pt bin
  // Seperately, but each file overlaid
  void Print1DHistogramsOverlayedDetaWFitRestricted( std::vector<std::vector<TH1F*> >& histograms, std::vector<std::vector<TF1*> >& fits, std::string outputDir, std::vector<std::string> analysisName, binSelector selector ) {
    
    // First, make the output directory if it doesnt exist
    boost::filesystem::path dir( outputDir.c_str() );
    boost::filesystem::create_directories( dir );
    
    TCanvas c1;
    c1.SetLeftMargin(0.15);
    c1.SetBottomMargin(0.2);
    for ( int i = 0; i < histograms[0].size(); ++i ) {
      
      double min, max;
      std::vector<TH1F*> tmpvec;
      for ( int j = 0; j < histograms.size(); ++j ) {
        tmpvec.push_back( histograms[j][i] );
      }
      
      FindGood1DUserRange( tmpvec, max, min );
      
      TLegend* leg = new TLegend(0.6, 0.6, .88, .88);
      leg->SetTextSize(0.04);
      
      for ( int j = 0; j < histograms.size(); ++j ) {
        
        histograms[j][i]->GetXaxis()->SetTitle("#Delta#eta");
        histograms[j][i]->GetXaxis()->SetTitleSize( 0.06 );
        histograms[j][i]->GetYaxis()->SetTitle( "1/N_{Dijet}dN/d#eta");
        histograms[j][i]->GetYaxis()->SetTitleSize( 0.04 );
        histograms[j][i]->SetTitle( selector.ptBinString[i].c_str() );
        histograms[j][i]->SetLineColor( j+1 );
        histograms[j][i]->SetMarkerStyle( j+20 );
        histograms[j][i]->SetMarkerColor( j+1 );
        histograms[j][i]->SetMarkerSize( 2 );
        histograms[j][i]->GetYaxis()->SetRangeUser( min, max );
        histograms[j][i]->GetXaxis()->SetRangeUser( -1.5, 1.5 );
        
        // try to set a fit function if it exists
        if ( histograms[j][i]->FindObject( fits[j][i]->GetName() ) ) {
          TF1* tmp = (TF1*) histograms[j][i]->FindObject( fits[j][i]->GetName() );
          tmp->SetLineColor(j+1);
        }
        
        if ( j == 0 ) {
          histograms[j][i]->Draw();
        }
        else {
          histograms[j][i]->Draw("same");
        }
        
        // add to legend
        if ( histograms.size() <= 2 )
          leg->AddEntry( histograms[j][i], selector.analysisStrings[j].c_str(), "lep" );
        else
          leg->AddEntry( histograms[j][i], analysisName[j].c_str(), "lep" );
        
      }
      leg->Draw();
      std::string tmp = outputDir + "/" + analysisName[0] + "_" + patch::to_string(i) + ".pdf";
      c1.SaveAs( tmp.c_str() );
      tmp = outputDir + "/" + analysisName[0] + "_" + patch::to_string(i) + ".C";
      c1.SaveAs( tmp.c_str() );
    }
    
    
  }


  
  void Print1DHistogramsOverlayedDphiOther( std::vector<TH1F*>& histograms, std::vector<TH1F*>& histograms2, std::string outputDir, std::string analysisName1, std::string analysisName2, binSelector selector ) {
   
    // First, make the output directory if it doesnt exist
    boost::filesystem::path dir( outputDir.c_str() );
    boost::filesystem::create_directories( dir );
    
    for ( int i = 0; i < histograms.size(); ++i ) {
      
      double min, max;
      std::vector<TH1F*> tmpvec;
      tmpvec.push_back( histograms[i] );
      tmpvec.push_back( histograms2[i] );
      FindGood1DUserRange( tmpvec, max, min );
      
      histograms[i]->GetXaxis()->SetTitle("#Delta#phi");
      histograms[i]->GetXaxis()->SetTitleSize( 0.06 );
      histograms[i]->GetYaxis()->SetTitle( "1/N_{Dijet}dN/d#phi");
      histograms[i]->GetYaxis()->SetTitleSize( 0.04 );
      histograms[i]->SetTitle( selector.ptBinString[i].c_str() );
      histograms[i]->SetLineColor( 1 );
      histograms[i]->SetMarkerStyle( 20 );
      histograms[i]->SetMarkerStyle( 2 );
      histograms[i]->SetMarkerColor( 1 );
      //histograms[i]->GetYaxis()->SetRangeUser( -1.0, 3.0 );
      
      histograms2[i]->GetXaxis()->SetTitle("#Delta#phi");
      histograms2[i]->GetXaxis()->SetTitleSize( 0.06 );
      histograms2[i]->GetYaxis()->SetTitle( "1/N_{Dijet}dN/d#phi");
      histograms2[i]->GetYaxis()->SetTitleSize( 0.04 );
      histograms2[i]->SetTitle( selector.ptBinString[i].c_str() );
      histograms2[i]->SetLineColor( 2 );
      histograms2[i]->SetMarkerStyle( 21 );
      histograms2[i]->SetMarkerStyle( 2 );
      histograms2[i]->SetMarkerColor( 2 );
      
      TLegend* leg = new TLegend(0.6, 0.6, .88, .88);
      leg->SetTextSize(0.04);
      
      leg->AddEntry( histograms[i], analysisName1.c_str(), "lep" );
      leg->AddEntry( histograms2[i], analysisName2.c_str(), "lep" );

      
      std::string tmp = outputDir + "/" + analysisName1 + "_" + patch::to_string(i) + ".pdf";
      
      TCanvas c1;
      c1.SetLeftMargin(0.15);
      c1.SetBottomMargin(0.2);
      histograms[i]->Draw();
      histograms2[i]->Draw("same");
      leg->Draw();
      c1.SaveAs( tmp.c_str() );
      tmp = outputDir + "/" + analysisName1 + "_" + patch::to_string(i) + ".C";
      c1.SaveAs( tmp.c_str() );
    }

    
  }
  
  // Used to print widths or yields
  // x = pt, y = yield/width
  void PrintGraphWithErrors( std::vector<std::vector<double> > x, std::vector<std::vector<double> > y, std::vector<std::vector<double> > x_err, std::vector<std::vector<double> > y_err, std::string outputDir, std::vector<std::string> analysisNames, std::string title, binSelector selector, const int pt_min, const int pt_max ) {
    
    // First, make the output directory if it doesnt exist
    boost::filesystem::path dir( outputDir.c_str() );
    boost::filesystem::create_directories( dir );
    
    int ptBins = pt_max - pt_min + 1 ;
    
    std::vector<TGraphErrors*> graphs;
    graphs.resize( x.size() );
    for ( int i = 0; i < x.size(); ++i ) {
      double xGraph[ptBins];
      double yGraph[ptBins];
      double xGraphErr[ptBins];
      double yGraphErr[ptBins];
  
      for ( int j = pt_min; j <= pt_max; ++j ) {
        xGraph[j-pt_min] = x[i][j];
        yGraph[j-pt_min] = y[i][j];
        xGraphErr[j-pt_min] = x_err[i][j];
        yGraphErr[j-pt_min] = y_err[i][j];
      }
      graphs[i] = new TGraphErrors( ptBins, xGraph, yGraph, xGraphErr, yGraphErr );
      graphs[i]->SetTitle( title.c_str() );
      graphs[i]->GetXaxis()->SetTitleSize( 0.06 );
      graphs[i]->GetXaxis()->SetTitle( "p_{T}" );
      graphs[i]->GetYaxis()->SetTitleSize( 0.04 );
      graphs[i]->GetYaxis()->SetTitle( "dN/dp_{T}" );
      graphs[i]->SetLineColor( i+1 );
      graphs[i]->SetMarkerColor( i+1 );
      graphs[i]->SetMarkerStyle( i+20 );
      graphs[i]->SetMarkerSize( 2 );
    }
    
    std::string tmp = outputDir + "/" + analysisNames[0] + "_graph.pdf";
    
    
    TCanvas c1;
    c1.SetLeftMargin(0.15);
    c1.SetBottomMargin(0.2);
    TLegend* leg = new TLegend( 0.6, 0.6, 0.88, 0.88 );
    leg->SetTextSize(0.04);
    
    for ( int i = 0; i < x.size(); ++i ) {
      
      if ( x.size() > 2 )
        leg->AddEntry( graphs[i], analysisNames[i].c_str(), "lep" );
      else
        leg->AddEntry( graphs[i], selector.analysisStrings[i].c_str(), "lep" );
      
      if ( i == 0 )
        graphs[i]->Draw();
      else
        graphs[i]->Draw("P");
      
    }
    leg->Draw();
    c1.SaveAs( tmp.c_str() );
    tmp = outputDir + "/" + analysisNames[0] + "_graph.C";
    c1.SaveAs( tmp.c_str() );
    
  }
  
  // printing with errors
  void Print1DDPhiHistogramsWithSysErr( std::vector<TH1F*>& histograms, std::vector<TH1F*>& errors, binSelector selector, std::string outputDir, double rangeLow, double rangeHigh  ) {
    
    // First, make the output directory if it doesnt exist
    boost::filesystem::path dir( outputDir.c_str() );
    boost::filesystem::create_directories( dir );
    
    if ( histograms.size() != errors.size() )
      __ERR("Warning: number of errors does not match number of signal histograms")
    
      double min, max;
    std::vector<TH1F*> tmpvec;
      
    TCanvas c1;
    for ( int i = 0; i < histograms.size(); ++i ) {
      if ( ! histograms[i] || !errors[i] ) {
        __ERR("Warning: Missing histogram. Skipping")
        continue;
      }
      
      tmpvec.clear();
      tmpvec.push_back( histograms[i]);
      tmpvec.push_back( errors[i] );
      
      FindGood1DUserRange( tmpvec, max, min, rangeHigh, rangeLow );
      
      std::string tmp = outputDir + "/" + "dphi_pt_" + patch::to_string(i) +"_err.pdf";

      histograms[i]->GetXaxis()->SetTitle("#Delta#phi");
      histograms[i]->GetXaxis()->SetTitleSize( 0.06 );
      histograms[i]->GetYaxis()->SetTitle( "1/N_{Dijet}dN/d#phi");
      histograms[i]->GetYaxis()->SetTitleSize( 0.04 );
      histograms[i]->SetTitle( selector.ptBinString[i].c_str() );
      histograms[i]->GetXaxis()->SetRangeUser( rangeLow, rangeHigh );
 
      errors[i]->GetXaxis()->SetTitle("#Delta#phi");
      errors[i]->GetYaxis()->SetTitle( "1/N_{Dijet}dN/d#phi");
      errors[i]->SetFillColorAlpha( kRed-10, 0.60 );
      errors[i]->SetFillStyle(1001);
      errors[i]->SetLineWidth( 0 );
      errors[i]->SetMarkerColor( 0 );
      errors[i]->GetXaxis()->SetRangeUser( rangeLow, rangeHigh );
      errors[i]->GetYaxis()->SetRangeUser( min, max );

      errors[i]->Draw("9e2");
      histograms[i]->Draw("9same");

      c1.SaveAs ( tmp.c_str() );
      tmp = outputDir + "/" + "dphi_pt_" + patch::to_string(i) +"_err.C";
      c1.SaveAs( tmp.c_str() );
    }
    
  }
  
  // printing with errors
  void Print1DDEtaHistogramsWithSysErr( std::vector<TH1F*>& histograms, std::vector<TH1F*>& errors, binSelector selector, std::string outputDir, double rangeLow, double rangeHigh  ) {
    
    // First, make the output directory if it doesnt exist
    boost::filesystem::path dir( outputDir.c_str() );
    boost::filesystem::create_directories( dir );
    
    if ( histograms.size() != errors.size() )
      __ERR("Warning: number of errors does not match number of signal histograms")
      
    
    double min, max;
    std::vector<TH1F*> tmpvec;
      
    TCanvas c1;
    for ( int i = 0; i < histograms.size(); ++i ) {
      
      if ( ! histograms[i] || !errors[i] ) {
        __ERR("Warning: Missing histogram. Skipping")
        continue;
      }
      
      tmpvec.clear();
      tmpvec.push_back( histograms[i]);
      tmpvec.push_back( errors[i] );
      
      FindGood1DUserRange( tmpvec, max, min, rangeHigh, rangeLow );
      
      
      std::string tmp = outputDir + "/" + "deta_pt_" + patch::to_string(i) +"_err.pdf";
      
      histograms[i]->GetXaxis()->SetTitle("#Delta#eta");
      histograms[i]->GetXaxis()->SetTitleSize( 0.06 );
      histograms[i]->GetYaxis()->SetTitle( "1/N_{Dijet}dN/d#eta");
      histograms[i]->GetYaxis()->SetTitleSize( 0.04 );
      histograms[i]->SetTitle( selector.ptBinString[i].c_str() );
      histograms[i]->GetXaxis()->SetRangeUser( rangeLow, rangeHigh );
      
      errors[i]->GetXaxis()->SetTitle("#Delta#eta");
      errors[i]->GetYaxis()->SetTitle( "1/N_{Dijet}dN/d#eta");
      errors[i]->SetFillColorAlpha( kRed-10, 0.60 );
      errors[i]->SetFillStyle(1001);
      errors[i]->SetLineWidth( 0 );
      errors[i]->SetMarkerColor( 0 );
      errors[i]->GetXaxis()->SetRangeUser( rangeLow, rangeHigh );
      errors[i]->GetYaxis()->SetRangeUser( min, max );
      
      errors[i]->Draw("9e2");
      histograms[i]->Draw("9same");
      
      c1.SaveAs ( tmp.c_str() );
      tmp = outputDir + "/" + "deta_pt_" + patch::to_string(i) +"_err.C";
      c1.SaveAs( tmp.c_str() );
    }
    
    
  }
  
  void Print1DDPhiHistogramsWithSysErr( std::vector<std::vector<TH1F*> >& histograms, std::vector<std::vector<TH1F*> >& errors, binSelector selector, std::string outputDir, double rangeLow, double rangeHigh  ) {
    
    // First, make the output directory if it doesnt exist
    boost::filesystem::path dir( outputDir.c_str() );
    boost::filesystem::create_directories( dir );
    
    if ( histograms.size() != errors.size() )
      __ERR("Warning: number of errors does not match number of signal histograms")
      
    
    TCanvas c1;
    for ( int i = 0; i < histograms.size(); ++i ) {
      
      double min, max;
      std::vector<TH1F*> tmpvec;
      
      for ( int j = 0; j < histograms[i].size(); ++j ) {
      
        if ( ! histograms[i][j] || !errors[i][j] ) {
          __ERR("Warning: Missing histogram. Skipping")
          continue;
        }
        
        tmpvec.clear();
        tmpvec.push_back( histograms[i][j]);
        tmpvec.push_back( errors[i][j] );
      
        FindGood1DUserRange( tmpvec, max, min, rangeHigh, rangeLow );
      
      
        std::string tmp = outputDir + "/" + "dphi_file_" + patch::to_string(i) + "_pt_" + patch::to_string(j) +"_err.pdf";
        
        histograms[i][j]->GetXaxis()->SetTitle("#Delta#phi");
        histograms[i][j]->GetXaxis()->SetTitleSize( 0.06 );
        histograms[i][j]->GetYaxis()->SetTitle( "1/N_{Dijet}dN/d#phi");
        histograms[i][j]->GetYaxis()->SetTitleSize( 0.04 );
        histograms[i][j]->SetTitle( selector.ptBinString[j].c_str() );
        histograms[i][j]->GetXaxis()->SetRangeUser( rangeLow, rangeHigh );
      
        errors[i][j]->GetXaxis()->SetTitle("#Delta#phi");
        errors[i][j]->GetYaxis()->SetTitle( "1/N_{Dijet}dN/d#phi");
        errors[i][j]->SetFillColorAlpha( kRed-10, 0.60 );
        errors[i][j]->SetFillStyle(1001);
        errors[i][j]->SetLineWidth( 0 );
        errors[i][j]->SetMarkerColor( 0 );
        errors[i][j]->GetXaxis()->SetRangeUser( rangeLow, rangeHigh );
        errors[i][j]->GetYaxis()->SetRangeUser( min, max );
      
        errors[i][j]->Draw("9e2");
        histograms[i][j]->Draw("9same");
      
        c1.SaveAs ( tmp.c_str() );
        tmp = outputDir + "/" + "dphi_file_" + patch::to_string(i) + "_pt_" + patch::to_string(j) +"_err.C";
        c1.SaveAs ( tmp.c_str() );
        
      }
    }

    
  }
  
  void Print1DDEtaHistogramsWithSysErr( std::vector<std::vector<TH1F*> >& histograms, std::vector<std::vector<TH1F*> >& errors, binSelector selector, std::string outputDir, double rangeLow, double rangeHigh  ) {
    
    // First, make the output directory if it doesnt exist
    boost::filesystem::path dir( outputDir.c_str() );
    boost::filesystem::create_directories( dir );
    
    if ( histograms.size() != errors.size() )
      __ERR("Warning: number of errors does not match number of signal histograms")
      
      
    TCanvas c1;
    for ( int i = 0; i < histograms.size(); ++i ) {
      double min, max;
      std::vector<TH1F*> tmpvec;

      for ( int j = 0; j < histograms[i].size(); ++j ) {
      
        if ( ! histograms[i][j] || !errors[i][j] ) {
          __ERR("Warning: Missing histogram. Skipping")
          continue;
        }
      
        tmpvec.clear();
        tmpvec.push_back( histograms[i][j]);
        tmpvec.push_back( errors[i][j] );
        
        FindGood1DUserRange( tmpvec, max, min, rangeHigh, rangeLow );
      
      
        std::string tmp = outputDir + "/" + "deta_" + patch::to_string(i) + "_pt_" + patch::to_string(j) + "_err.pdf";
        
        histograms[i][j]->GetXaxis()->SetTitle("#Delta#eta");
        histograms[i][j]->GetXaxis()->SetTitleSize( 0.06 );
        histograms[i][j]->GetYaxis()->SetTitle( "1/N_{Dijet}dN/d#eta");
        histograms[i][j]->GetYaxis()->SetTitleSize( 0.04 );
        histograms[i][j]->SetTitle( selector.ptBinString[j].c_str() );
        histograms[i][j]->GetXaxis()->SetRangeUser( rangeLow, rangeHigh );
      
        errors[i][j]->GetXaxis()->SetTitle("#Delta#eta");
        errors[i][j]->GetYaxis()->SetTitle( "1/N_{Dijet}dN/d#eta");
        errors[i][j]->SetFillColorAlpha( kRed-10, 0.60 );
        errors[i][j]->SetFillStyle(1001);
        errors[i][j]->SetLineWidth( 0 );
        errors[i][j]->SetMarkerColor( 0 );
        errors[i][j]->GetXaxis()->SetRangeUser( rangeLow, rangeHigh );
        errors[i][j]->GetYaxis()->SetRangeUser( min, max );
      
        errors[i][j]->Draw("9e2");
        histograms[i][j]->Draw("9same");
        
        c1.SaveAs ( tmp.c_str() );
        tmp = outputDir + "/" + "deta_" + patch::to_string(i) + "_pt_" + patch::to_string(j) + "_err.C";
        c1.SaveAs( tmp.c_str() );
      }
    }
  }
  
  // and the plotting with full set of errors
  void Print1DDPhiHistogramsWithSysErrFull( std::vector<std::vector<TH1F*> >& histograms, std::vector<std::vector<TH1F*> >& errors, std::vector<TH1F*>& errors2, binSelector selector, std::string outputDir, std::vector<std::string> text, double rangeLow, double rangeHigh  ) {
    
    // First, make the output directory if it doesnt exist
    boost::filesystem::path dir( outputDir.c_str() );
    boost::filesystem::create_directories( dir );
    
    if ( histograms.size() != errors.size() )
      __ERR("Warning: number of errors does not match number of signal histograms")
      
    if ( histograms[0].size() != errors[0].size() )
      __ERR("Warning: number of errors does not match number of signal histograms")

      
    TCanvas c1;
    c1.SetLeftMargin( 0.12 );
    c1.SetBottomMargin( 0.15 );
    for ( int i = 0; i < histograms[0].size(); ++i ) {
      
      double min, max;
      std::vector<TH1F*> tmpvec;
      tmpvec.push_back( histograms[0][i] );
      tmpvec.push_back( histograms[1][i] );
      tmpvec.push_back( errors[0][i] );
      tmpvec.push_back( errors[1][i] );
      tmpvec.push_back( errors2[i] );
      
      FindGood1DUserRange( tmpvec, max, min, rangeHigh, rangeLow );
      
      for ( int j = 0; j < histograms.size(); ++j ) {
        
        histograms[j][i]->GetXaxis()->SetTitle("#Delta#phi");
        histograms[j][i]->GetXaxis()->SetTitleSize( 0.06 );
        histograms[j][i]->GetYaxis()->SetTitle( "1/N_{Dijets}dN/d#Delta#phi");
        histograms[j][i]->GetYaxis()->SetTitleSize( 0.04 );
        histograms[j][i]->SetTitle( selector.ptBinString[i].c_str() );
        histograms[j][i]->GetXaxis()->SetRangeUser( rangeLow, rangeHigh );
        
        errors[j][i]->GetXaxis()->SetTitle("#Delta#phi");
        errors[j][i]->GetXaxis()->SetTitleSize( 0.06 );
        errors[j][i]->GetYaxis()->SetTitle( "1/N_{Dijets}dN/d#Delta#phi");
        errors[j][i]->GetYaxis()->SetTitleSize( 0.06 );
        errors[j][i]->SetFillColorAlpha( 2, 0.60 );
        if ( j == 0 ) {
          errors[j][i]->SetFillColorAlpha( 1, 0.35 );
        }
        errors[j][i]->SetFillStyle(1001);
        errors[j][i]->SetLineWidth( 0 );
        errors[j][i]->SetMarkerColor( 0 );
        errors[j][i]->GetXaxis()->SetRangeUser( rangeLow, rangeHigh );
        errors[j][i]->GetYaxis()->SetRangeUser( min, max );
        
        errors2[i]->SetFillStyle(1001);
        errors2[i]->SetLineWidth(0);
        errors2[i]->SetMarkerSize(0);
        errors2[i]->SetFillColorAlpha( 46, 0.30 );
        errors2[i]->GetXaxis()->SetRangeUser( rangeLow, rangeHigh );
        // changed to set constant range
        //errors2[i]->GetYaxis()->SetRangeUser( min, max );
        errors2[i]->GetYaxis()->SetRangeUser( -1, 5 );
        errors2[i]->GetXaxis()->SetTitle("#Delta#phi");
        errors2[i]->GetXaxis()->SetTitleSize( 0.075 );
        errors2[i]->GetXaxis()->SetTitleOffset( 0.80 );
        errors2[i]->GetXaxis()->CenterTitle( false );
        errors2[i]->GetXaxis()->SetLabelSize( 0.06 );
        errors2[i]->GetYaxis()->SetTitle( "1/N_{Dijets}dN/d#Delta#phi");
        errors2[i]->GetYaxis()->CenterTitle( true );
        errors2[i]->GetYaxis()->SetTitleSize( 0.065 );
        errors2[i]->GetYaxis()->SetTitleOffset( 0.7 );
        errors2[i]->GetYaxis()->SetLabelSize( 0.06 );
        
      }
      
      errors2[i]->Draw("9e2");
      errors[0][i]->Draw("9e2SAME");
      errors[1][i]->Draw("9e2SAME");
      histograms[0][i]->Draw("9SAME");
      histograms[1][i]->Draw("9SAME");
      
      TLegend* leg = new TLegend( 0.55, 0.6, 0.88, 0.88 );
      leg->SetTextSize(0.045);
      
      leg->AddEntry( histograms[0][i], "AuAu HT 0-20%", "lep" );
      leg->AddEntry( histograms[1][i], "p+p HT", "lep" );
      leg->AddEntry( errors[0][i], "tracking unc. Au+Au", "f" );
      leg->AddEntry( errors[1][i], "tracking unc. p+p", "f" );
      leg->AddEntry( errors2[i], "JES unc.", "f");
      leg->Draw();
      
      // and draw some titles and such
      TPaveText *t = new TPaveText(0.1, 0.6, 0.47, 0.8, "NB NDC");
      t->SetFillStyle(0);
      t->SetBorderSize(0);
      t->AddText( selector.ptBinString[i].c_str() );
      for ( int k = 0; k < text.size(); ++k ) {
        t->AddText( text[k].c_str() );
      }
      
      t->Draw();
      
      // and STAR preliminary message
      TLatex latex;
      latex.SetNDC();
      latex.SetTextSize(0.045);
      // latex.SetTextColor(kGray+3);
      latex.SetTextColor(kRed+3);
      latex.DrawLatex( 0.16, 0.84, "STAR Preliminary");
      
      std::string tmp = outputDir + "/" + "dphi_pt_" + patch::to_string(i) +"_full.pdf";
      c1.SaveAs( tmp.c_str() );
      tmp = outputDir + "/" + "dphi_pt_" + patch::to_string(i) +"_full.C";
      c1.SaveAs( tmp.c_str() );
    }
  }
  
  void Print1DDEtaHistogramsWithSysErrFull( std::vector<std::vector<TH1F*> >& histograms, std::vector<std::vector<TH1F*> >& errors, std::vector<TH1F*>& errors2, binSelector selector, std::string outputDir, std::vector<std::string> text, double rangeLow, double rangeHigh  ) {
    
    // First, make the output directory if it doesnt exist
    boost::filesystem::path dir( outputDir.c_str() );
    boost::filesystem::create_directories( dir );
    
    if ( histograms.size() != errors.size() )
      __ERR("Warning: number of errors does not match number of signal histograms")
      
      
    if ( histograms[0].size() != errors[0].size() )
      __ERR("Warning: number of errors does not match number of signal histograms")
      
      
      TCanvas c1;
    c1.SetLeftMargin( 0.12 );
    c1.SetBottomMargin( 0.15 );
    for ( int i = 0; i < histograms[0].size(); ++i ) {
      
      double min, max;
      std::vector<TH1F*> tmpvec;
      tmpvec.push_back( histograms[0][i] );
      tmpvec.push_back( histograms[1][i] );
      tmpvec.push_back( errors[0][i] );
      tmpvec.push_back( errors[1][i] );
      tmpvec.push_back( errors2[i] );
      
      FindGood1DUserRange( tmpvec, max, min, rangeHigh, rangeLow );
      
      
      for ( int j = 0; j < histograms.size(); ++j ) {
        
        histograms[j][i]->GetXaxis()->SetTitle("#Delta#eta");
        histograms[j][i]->GetXaxis()->SetTitleSize( 0.06 );
        histograms[j][i]->GetYaxis()->SetTitle( "1/N_{Dijets}dN/d#Delta#eta");
        histograms[j][i]->GetYaxis()->SetTitleSize( 0.04 );
        histograms[j][i]->SetTitle( selector.ptBinString[i].c_str() );
        histograms[j][i]->GetXaxis()->SetRangeUser( rangeLow, rangeHigh );
        
        errors[j][i]->GetXaxis()->SetTitle("#Delta#eta");
        errors[j][i]->GetXaxis()->SetTitleSize( 0.06 );
        errors[j][i]->GetYaxis()->SetTitle( "1/N_{Dijets}dN/d#Delta#eta");
        errors[j][i]->GetYaxis()->SetTitleSize( 0.06 );
        errors[j][i]->SetFillColorAlpha( 2, 0.60 );
        if ( j == 0 ) {
          errors[j][i]->SetFillColorAlpha( 1, 0.35 );
        }
        errors[j][i]->SetFillStyle(1001);
        errors[j][i]->SetLineWidth( 0 );
        errors[j][i]->SetMarkerColor( 0 );
        errors[j][i]->GetXaxis()->SetRangeUser( rangeLow, rangeHigh );
        errors[j][i]->GetYaxis()->SetRangeUser( min, max );
        
        errors2[i]->SetFillStyle(1001);
        errors2[i]->SetLineWidth(0);
        errors2[i]->SetMarkerSize(0);
        errors2[i]->SetFillColorAlpha( 46, 0.30 );
        errors2[i]->GetXaxis()->SetRangeUser( rangeLow, rangeHigh );
        // changed to set constant range
        //errors2[i]->GetYaxis()->SetRangeUser( min, max );
        errors2[i]->GetYaxis()->SetRangeUser( -1, 5 );
        errors2[i]->GetXaxis()->SetTitle("#Delta#eta");
        errors2[i]->GetXaxis()->SetTitleSize( 0.075 );
        errors2[i]->GetXaxis()->SetTitleOffset( 0.80 );
        errors2[i]->GetXaxis()->CenterTitle( false );
        errors2[i]->GetXaxis()->SetLabelSize( 0.06 );
        errors2[i]->GetYaxis()->SetTitle( "1/N_{Dijets}dN/d#Delta#eta");
        errors2[i]->GetYaxis()->CenterTitle( true );
        errors2[i]->GetYaxis()->SetTitleSize( 0.065 );
        errors2[i]->GetYaxis()->SetTitleOffset( 0.7 );
        errors2[i]->GetYaxis()->SetLabelSize( 0.06 );
      }
      
      errors2[i]->Draw("9e2");
      errors[0][i]->Draw("9e2SAME");
      errors[1][i]->Draw("9e2SAME");
      histograms[0][i]->Draw("9SAME");
      histograms[1][i]->Draw("9SAME");
      
      TLegend* leg = new TLegend( 0.55, 0.6, 0.88, 0.88 );
      leg->SetTextSize(0.045);
      
      leg->AddEntry( histograms[0][i], "AuAu HT 0-20%", "lep" );
      leg->AddEntry( histograms[1][i], "p+p HT", "lep" );
      leg->AddEntry( errors[0][i], "tracking unc. Au+Au", "f" );
      leg->AddEntry( errors[1][i], "tracking unc. p+p", "f" );
      leg->AddEntry( errors2[i], "JES unc.", "f");
      leg->Draw();
      
      // and draw some titles and such
      TPaveText *t = new TPaveText(0.1, 0.6, 0.47, 0.8, "NB NDC");
      t->SetFillStyle(0);
      t->SetBorderSize(0);
      t->AddText( selector.ptBinString[i].c_str() );
      for ( int k = 0; k < text.size(); ++k ) {
        t->AddText( text[k].c_str() );
      }
      
      t->Draw();
      
      // and STAR preliminary message
      TLatex latex;
      latex.SetNDC();
      latex.SetTextSize(0.045);
      // latex.SetTextColor(kGray+3);
      latex.SetTextColor(kRed+3);
      latex.DrawLatex( 0.16, 0.84, "STAR Preliminary");
      
      std::string tmp = outputDir + "/" + "deta_pt_" + patch::to_string(i) +"_full.pdf";
      c1.SaveAs( tmp.c_str() );
      tmp = outputDir + "/" + "deta_pt_" + patch::to_string(i) +"_full.C";
      c1.SaveAs( tmp.c_str() );
    }

  }
  
  // Used to make some pretty plots of dPhi near/far
  void PrintNearFarDPhiCorrelations( std::vector<TH1F*> hist1, std::vector<TH1F*> hist2, binSelector selector, std::string outputDir, std::vector<std::string> text, double rangeLow, double rangeHigh ) {
    // First, make the output directory if it doesnt exist
    boost::filesystem::path dir( outputDir.c_str() );
    boost::filesystem::create_directories( dir );
    
    TCanvas c1;
    c1.SetLeftMargin( 0.12 );
    c1.SetBottomMargin( 0.15 );
    for ( int i = 0; i < hist1.size(); ++i ) {
      std::string tmp = outputDir + "/" + "dphi_nearfar_pt_" + patch::to_string(i) +"_full.pdf";
      std::string tmpC = outputDir + "/" + "dphi_nearfar_pt_" + patch::to_string(i) +"_full.C";
      
      hist1[i]->GetXaxis()->SetTitle("#Delta#phi");
      hist1[i]->GetXaxis()->SetTitleSize( 0.075 );
      hist1[i]->GetXaxis()->SetTitleOffset( 0.80 );
      hist1[i]->GetXaxis()->CenterTitle( false );
      hist1[i]->GetXaxis()->SetLabelSize( 0.06 );
      hist1[i]->GetYaxis()->SetTitle( "dY/d#Delta#phi");
      hist1[i]->GetYaxis()->SetTitleSize( 0.065 );
      hist1[i]->GetYaxis()->SetTitleOffset( 0.7 );
      hist1[i]->GetYaxis()->CenterTitle( true );
      hist1[i]->GetYaxis()->SetLabelSize( 0.06 );
      //hist1[i]->SetTitle( selector.ptBinString[i].c_str() );
      hist1[i]->GetXaxis()->SetRangeUser( rangeLow, rangeHigh );
      hist1[i]->SetMarkerStyle( 20 );
      hist1[i]->SetMarkerColor( 1 );
      hist1[i]->SetMarkerSize( 2 );
      hist1[i]->SetLineColor( 1 );
      
      hist2[i]->GetXaxis()->SetTitle("#Delta#phi");
      hist2[i]->GetXaxis()->SetTitleSize( 0.06 );
      hist2[i]->GetYaxis()->SetTitle( "1/N_{Dijet}dN/d#Delta#phi");
      hist2[i]->GetYaxis()->SetTitleSize( 0.04 );
      //hist1[i]->SetTitle( selector.ptBinString[i].c_str() );
      hist2[i]->GetXaxis()->SetRangeUser( rangeLow, rangeHigh );
      hist2[i]->SetMarkerStyle( 21 );
      hist2[i]->SetMarkerColor( 2 );
      hist2[i]->SetLineColor( 2 );
      hist2[i]->SetMarkerSize( 2 );
      
      hist1[i]->Draw();
      hist2[i]->Draw("SAME");
      
      TLegend* leg = new TLegend( 0.6, 0.7, 0.88, 0.88 );
      leg->SetTextSize(0.045);
      
      leg->AddEntry( hist1[i], "|#Delta#eta|<0.71", "lep" );
      leg->AddEntry( hist2[i], "0.71<|#Delta#eta|<1.0", "lep" );
      leg->Draw();
      
      // and draw some titles and such
      TPaveText *t = new TPaveText(0.12, 0.7, 0.48, 0.88, "NB NDC");
      t->SetFillStyle(0);
      t->SetBorderSize(0);
      
      t->AddText( selector.ptBinString[i].c_str() );
      
      for ( int j = 0; j < text.size(); ++j ) {
        t->AddText( text[j].c_str() );
      }
      
      t->Draw();
      
      // and STAR preliminary message
      TLatex latex;
      latex.SetNDC();
      latex.SetTextSize(0.045);
      // latex.SetTextColor(kGray+3);
      latex.SetTextColor(kRed+3);
      //latex.DrawLatex( 0.15, 0.64, "STAR Preliminary");
      
      
      c1.SaveAs( tmp.c_str() );
      c1.SaveAs( tmpC.c_str() );
    }
    
  }
  
  
  // printing some graphs with some systematic errors as well
  void PrintGraphsWithSystematics( std::vector<TGraphErrors*>& graphs, std::vector<TGraphErrors*>& sys1, std::vector<TGraphErrors*> sys2, std::string outputDir, std::vector<std::string> analysisNames, std::string title, binSelector selector ) {
    
    // First, make the output directory if it doesnt exist
    boost::filesystem::path dir( outputDir.c_str() );
    boost::filesystem::create_directories( dir );
    
    if ( graphs.size() != 2 || sys1.size() != 2 || sys2.size()!= 1 ) {
      __ERR("WARNING: we arent prepared for this combination!!")
      return;
    }
    
    
    
    TCanvas c1;
    c1.SetBottomMargin( 0.15 );
    c1.SetLeftMargin( 0.12 );
    //c1.DrawFrame(-2, 20, -2, 20);
    for ( int i = 0; i < 2; ++i ) {
      
      
      
      graphs[i]->SetTitle( title.c_str() );
      graphs[i]->GetXaxis()->SetTitleSize( 0.055 );
      graphs[i]->GetXaxis()->SetTitleOffset( 0.98 );
      graphs[i]->GetXaxis()->SetTitle( "p_{T} (GeV/c)" );
      graphs[i]->GetXaxis()->SetLabelSize( 0.06 );
      graphs[i]->GetYaxis()->SetTitleSize( 0.065 );
      graphs[i]->GetYaxis()->SetTitleOffset( 0.7 );
      graphs[i]->GetYaxis()->SetTitle( "dY/dp_{T} (GeV/c)^{-1}" );
      graphs[i]->GetYaxis()->CenterTitle( true );
      graphs[i]->GetYaxis()->SetLabelSize( 0.050 );
      graphs[i]->SetLineColor( i+1 );
      graphs[i]->SetMarkerColor( i+1 );
      graphs[i]->SetMarkerStyle( i+20 );
      graphs[i]->SetMarkerSize( 2 );
      graphs[i]->GetYaxis()->SetRangeUser( 0, 2 );
      
      sys1[i]->SetFillColorAlpha( i+1, 0.35 );
      sys1[i]->SetFillStyle(1001);
      
      sys1[i]->SetMarkerSize( 0 );
      sys1[i]->SetLineWidth( 0 );
      
      if ( i == 0 ) {
        sys1[i]->SetFillStyle(1001);
        sys1[i]->SetFillColorAlpha(i+1, 0.6);
      }
      
      if ( i == 1 ) {
        sys1[i]->SetFillStyle(1001);
        sys1[i]->SetFillColorAlpha(i+1, 0.6);
      }
      
      if ( i == 0 ) {
        sys2[0]->SetFillStyle(1001);
        sys2[0]->SetFillColorAlpha( 46, 0.30 );
      }
      
    }
    graphs[0]->Draw();
    graphs[1]->Draw("P");
    sys2[0]->Draw("3");
    sys1[0]->Draw("3");
    sys1[1]->Draw("3");
    
    TLegend* leg = new TLegend( 0.5, 0.5, 0.88, 0.78 );
    
    leg->AddEntry( graphs[0], "AuAu HT 0-20%", "lep" );
    leg->AddEntry( graphs[1], "p+p HT", "lep" );
    leg->AddEntry( sys1[0], "tracking unc. Au+Au", "f" );
    leg->AddEntry( sys1[1], "tracking unc. p+p", "f" );
    leg->AddEntry( sys2[0], "JES unc.", "f");
    leg->Draw();
    
    // and draw some titles and such
    TPaveText *t = new TPaveText(0.5, 0.8, 0.88, 0.88, "NB NDC");
    t->SetFillStyle(0);
    t->SetBorderSize(0);
    t->AddText( title.c_str() );
    
    t->Draw();
    
    // and STAR preliminary message
    TLatex latex;
    latex.SetNDC();
    latex.SetTextSize(0.045);
    // latex.SetTextColor(kGray+3);
    latex.SetTextColor(kRed+3);
    latex.DrawLatex( 0.2, 0.84, "STAR Preliminary");
    

    
    std::string tmp = outputDir + "/" + analysisNames[0] + "_graph.pdf";
    c1.SaveAs( tmp.c_str() );
    tmp = outputDir + "/" + analysisNames[0] + "_graph.C";
    c1.SaveAs( tmp.c_str() );
  }
  
  void PrintGraphsWithSystematics( std::vector<TGraphErrors*>& graphs, std::vector<TGraphErrors*>& sys1, std::vector<TGraphErrors*> sys2, std::vector<TGraphErrors*> sys3, std::string outputDir, std::vector<std::string> analysisNames, std::string title, binSelector selector ) {
    
    // First, make the output directory if it doesnt exist
    boost::filesystem::path dir( outputDir.c_str() );
    boost::filesystem::create_directories( dir );
    
    if ( graphs.size() != 2 || sys1.size() != 2 || sys2.size()!= 1 || sys3.size() != 2 ) {
      __ERR("WARNING: we arent prepared for this combination!!")
      return;
    }
    
    
    
    TCanvas c1;
    //c1.DrawFrame(-2, 20, -2, 20);
    for ( int i = 0; i < 2; ++i ) {
      
      
      
      graphs[i]->SetTitle( title.c_str() );
      graphs[i]->GetXaxis()->SetTitleSize( 0.06 );
      graphs[i]->GetXaxis()->SetTitle( "p_{T}" );
      graphs[i]->GetYaxis()->SetTitleSize( 0.04 );
      graphs[i]->GetYaxis()->SetTitle( "dY/dp_{T}" );
      graphs[i]->SetLineColor( i+1 );
      graphs[i]->SetMarkerColor( i+1 );
      graphs[i]->SetMarkerStyle( i+20 );
      graphs[i]->SetMarkerSize( 2 );
      
      sys1[i]->SetFillColorAlpha( i+1, 0.35 );
      sys1[i]->SetFillStyle(1001);
      
      sys1[i]->SetMarkerSize( 0 );
      sys1[i]->SetLineWidth( 0 );
      
      if ( i == 0 ) {
        sys1[i]->SetFillStyle(1001);
        sys1[i]->SetFillColorAlpha(i+1, 0.35);
        sys2[0]->SetFillStyle(1001);
        sys2[0]->SetFillColorAlpha( 16, 0.35 );
        sys3[i]->SetFillStyle( 1001 );
        sys3[i]->SetFillColorAlpha( 40, 0.35 );
        
      }
      
      if ( i == 1 ) {
        sys1[i]->SetFillStyle(1001);
        sys1[i]->SetFillColorAlpha(i+1, 0.34);
        sys3[i]->SetFillColorAlpha(20, 0.35);
        sys3[i]->SetFillStyle( 1001  );
      }
      
      
    }
    graphs[0]->Draw();
    graphs[1]->Draw("P");
    sys1[0]->Draw("3");
    sys1[1]->Draw("3");
    sys2[0]->Draw("3");
    sys3[0]->Draw("3");
    sys3[1]->Draw("3");
    
    TLegend* leg = new TLegend( 0.5, 0.6, 0.88, 0.88 );
    
    leg->AddEntry( graphs[0], "AuAu HT 0-20%", "lep" );
    leg->AddEntry( graphs[1], "p+p HT", "lep" );
    leg->AddEntry( sys1[0], "tracking uncertainty Au+Au", "f" );
    leg->AddEntry( sys1[1], "tracking uncertainty p+p", "f" );
    leg->AddEntry( sys2[0], "jet energy scale uncertainty", "f");
    leg->AddEntry( sys3[0], "Sys Uncertainty projection range AuAu", "f" );
    leg->AddEntry( sys3[0], "Sys Uncertainty projection range pp", "f");
    leg->Draw();
    
    
    std::string tmp = outputDir + "/" + analysisNames[0] + "_graph.pdf";
    c1.SaveAs( tmp.c_str() );
    tmp = outputDir + "/" + analysisNames[0] + "_graph.C";
    c1.SaveAs( tmp.c_str() );
  }

  void PrintPPHardOverlay( std::vector<TH1F*>& hist1, std::vector<TH1F*>& hist2,  std::string outputDir, binSelector selector  ) {
    
    // First, make the output directory if it doesnt exist
    boost::filesystem::path dir( outputDir.c_str() );
    boost::filesystem::create_directories( dir );
    
    for ( int i = 0; i < hist1.size(); ++i ) {
      
      std::string tmp = outputDir + "/" + "overlaid_pt_" + patch::to_string(i) + ".pdf";
      hist1[i]->SetLineColor( kBlack );
      hist1[i]->SetMarkerSize( 2 );
      hist1[i]->SetMarkerColor( kBlack );
      hist1[i]->SetMarkerStyle( 21 );
      hist1[i]->GetXaxis()->SetRangeUser( -1, 1 );
      
      hist2[i]->SetMarkerColor( kRed );
      hist2[i]->SetMarkerSize( 2 ) ;
      hist2[i]->SetLineColor( kBlack );
      hist2[i]->SetMarkerStyle( 22 );
      
      TCanvas c1;
      hist1[i]->Draw();
      hist2[i]->Draw("SAME");
      c1.SaveAs( tmp.c_str() );
      
      
    }
    
  }
  
} // end namespace
