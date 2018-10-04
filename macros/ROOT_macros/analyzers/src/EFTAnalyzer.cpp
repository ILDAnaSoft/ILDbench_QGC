#include "../include/EFTAnalyzer.h"

//-------------------------------------------------------------------------------------------------

void EFTAnalyzer::findParameterPoints( string weight_directory ){
  // WARNING: Uses hardcoded file name conventions!
  if (weight_directory == "") {
    cout << "ERROR in findParameterPoints: Empty input path!" << endl;
    return;
  }
  
  InputManager parameter_file_searcher;
  parameter_file_searcher.setInputDirectory( weight_directory );
  parameter_file_searcher.setFilenameExtension( "py" );
  parameter_file_searcher.findFiles();
  
  vector<string> parameter_file_paths {};
  parameter_file_searcher.getFilePaths( parameter_file_paths );
  
  string parameter_path = "";
  if ( parameter_file_paths.size() > 1 ){
    cout  << "ERROR in findParameterPoints: found multiple parameter (root) files in " 
          << weight_directory << endl;
  } else if ( parameter_file_paths.size() == 0 ) {
    cout  << "In findParameterPoints: no parameter files in " << weight_directory << endl;
  } else {
    cout  << "In findParameterPoints: found parameter file in " << weight_directory << endl;
    parameter_path = parameter_file_paths[0];
  }
  
  vector<vector<float>> parameter_matrix = PyHelp::read2DMatrixFromFile( parameter_path );
  // Remove point (0,0) -> Stupid convention made by me...
  vector<float> zero_point {0,0};
  parameter_matrix.erase(remove(parameter_matrix.begin(),parameter_matrix.end(), zero_point),parameter_matrix.end());
  
  map<int, ParameterPoint> parameter_points{};
  for (int i=0;i<parameter_matrix.size();i++) {
    vector<float> parameters = parameter_matrix[i];
    if ( parameters.size() != 2 ) {
      cout << "ERROR in findParameterPoints: not two parameters for setting!!!" << endl;
      exit(1);
    } 
    parameter_points[i+1] = make_pair(parameters[0], parameters[1]);
  }
  
  if (m_parameter_points.empty()) {
    m_parameter_points = parameter_points;
    cout << "Found parameter points!" << endl;
  } else if ( m_parameter_points != parameter_points ) {
    cout << "ERROR in findParameterPoints: inconsistent with previous file!" << endl;
    exit(1);
  }
}

//-------------------------------------------------------------------------------------------------

void EFTAnalyzer::setupDummyWeightsFile( string weight_file_path ){
  if (weight_file_path == "") {
    cout << "ERROR in setupDummyWeightsFile: Empty input path!" << endl;
    return;
  }
  
  if ( m_dummy_weights_file != "" ) { return; }
  
  TFile *weight_file = TFile::Open(weight_file_path.c_str());
  TTree *weight_tree = (TTree*)weight_file->Get("weightsTree");
  
  m_dummy_weights_file = "/afs/desy.de/group/flc/pool/beyerjac/tmp/dummy_weights.root";
  
  int N_entries   = weight_tree->GetEntries();
  int N_branches  = weight_tree->GetNbranches(); 
  TObjArray *branch_list = weight_tree->GetListOfBranches();
  string branch_names [N_branches];
  
  for (int branch=0; branch<N_branches; branch++) {
    branch_names[branch] = branch_list->At(branch)->GetName();
  }
  weight_file->Close();
  delete weight_file;
  
  TFile *dummy_file = new TFile(m_dummy_weights_file.c_str(), "recreate");
  TTree* dummy_weight_tree = new TTree("weightsTree", "weightsTree");
  float weight_ptrs [N_branches];
  
  // Create same branches as other file had
  for (int branch=0; branch<N_branches; branch++) {
    string branch_name = branch_names[branch];
    dummy_weight_tree->Branch( branch_name.c_str(), &weight_ptrs[branch], (branch_name + "/F").c_str());
    weight_ptrs[branch] = 1.0;
  }
  
  for (int entry=0; entry<N_entries; entry++) {
    dummy_weight_tree->Fill();
  }
  
  dummy_file->cd();
  dummy_weight_tree->Write();
  dummy_file->Close();
  delete dummy_file;
}

//-------------------------------------------------------------------------------------------------

void EFTAnalyzer::searchForWeightFile( string file_path ){
  // Uses weight file convention to search for weight file to event file
  // WARNING: Has convention hardcoded!
  
  string file_directory = file_path.substr(0, file_path.find_last_of('/'));
  string weight_directory = file_directory + "/rescan_output";
  
  InputManager weight_file_searcher;
  weight_file_searcher.setInputDirectory( weight_directory );
  weight_file_searcher.setFilenameExtension( "root" );
  weight_file_searcher.findFiles();
  
  vector<string> weight_file_paths {};
  weight_file_searcher.getFilePaths( weight_file_paths );
  
  string weight_path = "";
  if ( weight_file_paths.size() > 1 ){
    cout  << "ERROR in searchForWeightFile: found multiple weight (root) files in " 
          << weight_directory << endl;
  } else if ( weight_file_paths.size() == 0 ) {
    cout  << "In searchForWeightFile: no weight files for " << file_path << endl;
  } else {
    cout  << "In searchForWeightFile: found weight file for " << file_path << endl;
    weight_path = weight_file_paths[0];
    
    this->findParameterPoints(weight_directory);
    this->setupDummyWeightsFile(weight_path);
  }
  
  FilePair events_weights_pair = make_pair( file_path, weight_path );
  m_events_weights_pairs.push_back(events_weights_pair);
}

//-------------------------------------------------------------------------------------------------

void EFTAnalyzer::searchForWeightFiles() {
  for ( auto const& file_path: m_input_file_paths ) {
    // Must be done separately first separately to create dummy weight file
    this->searchForWeightFile(file_path);
  }
}

//-------------------------------------------------------------------------------------------------

void EFTAnalyzer::getCombinedDataframe() {
  TChain* info_chain  = new TChain("processInfoTree");
  TChain* reco_chain  = new TChain("recoObservablesTree");
  TChain* mc_chain    = new TChain("mcObservablesTree");
  TChain* truth_chain = new TChain("mcTruthTree");
  TChain* weight_chain = new TChain("weightsTree");
  
  for ( auto const& events_weights_pair: m_events_weights_pairs ) {
    string file_path = events_weights_pair.first;
    string weight_path = events_weights_pair.second;
    
    // Check if weight file found, if not use Dummy weights (all 1)
    if (weight_path == "") {
      weight_path = m_dummy_weights_file;
    }
    
    info_chain  ->Add( file_path.c_str() );
    reco_chain  ->Add( file_path.c_str() );
    mc_chain    ->Add( file_path.c_str() );
    truth_chain ->Add( file_path.c_str() );
    weight_chain->Add( weight_path.c_str() );
  }
  
  this->findAllFinalStates( info_chain );
  
  info_chain->AddFriend(reco_chain, "reco");
  info_chain->AddFriend(mc_chain, "mcobs");
  info_chain->AddFriend(truth_chain, "mctruth");
  info_chain->AddFriend(weight_chain, "weights");
  RDataFrame* befriended_dataframe = new RDataFrame(*info_chain);
  m_dataframe = befriended_dataframe;
  
  m_all_chains = {info_chain, reco_chain, mc_chain, truth_chain, weight_chain};
}

//-------------------------------------------------------------------------------------------------

void EFTAnalyzer::run(){
  cout << "Searching for weight files." << endl;
  this->searchForWeightFiles();
  cout << "Reading in trees from files to create dataframe." << endl;
  this->getCombinedDataframe();
  cout << "Starting analysis." << endl;
  this->performAnalysis();
  cout << "Analysis finished - cleaning up." << endl;
  this->clearMemory();
  cout << "Done!" << endl;
}

//-------------------------------------------------------------------------------------------------

 pair<float, float> EFTAnalyzer::getParameterPoint( int setting_index ) const {
  return m_parameter_points.at(setting_index);
}

//-------------------------------------------------------------------------------------------------

float EFTAnalyzer::getEventWeight( float event_weight, float process_weight ){
  return event_weight * process_weight;
}

//-------------------------------------------------------------------------------------------------

function<float (float, float)> EFTAnalyzer::getEventWeightLambda(){
  return[this](float event_weight, float process_weight) {
    return this->getEventWeight( event_weight, process_weight ); 
  };
}

//-------------------------------------------------------------------------------------------------
