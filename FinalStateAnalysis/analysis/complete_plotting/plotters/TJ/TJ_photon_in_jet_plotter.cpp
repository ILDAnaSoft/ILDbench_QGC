#include "../../include/plotter.h"

// Very simple example

class TJPhotonsInJetsPlotter : public Plotter {

	void set_plotter_settings() {
		set_output_folder_name("TJ/TJ_photons_in_jets");	
		set_1D_array_reading( true );
		set_2D_array_reading( true );

	}

	vector<string> particle_types = {"all_q", "udsc", "b"};

	vector<string> Ngamma_classes = 	{"0", 	"1", "2", "3", 	"geq4"};
	
	string Ngamma_class_from_int ( int Ngamma ) {
		vector<int> Ngamma_min =			{0,		1,		2,		3, 		4};
		vector<int> Ngamma_max =			{0,		1,		2,		3,		max_Nparticles};

		for (int iclass=0; iclass<Ngamma_classes.size(); iclass++) {
			if ( (Ngamma >= Ngamma_min[iclass]) && (Ngamma <= Ngamma_max[iclass]) ) {
				return Ngamma_classes[iclass];		
			}
		}
		
		return "-1";

	}

	vector<string> gamma_classes = {"highE", "lowE"};
	vector<string> gamma_class_labels = {"E>5GeV", "E<5GeV"};

	vector<int> gamma_E_classes = {0, 5, 10, 50, 100};

	vector<string> levels = {"fe_tos", "tos_s", "t_tos"};

	vector<string> level1_descr = {
		"final elementon", 
		"detected particles true values",
		"all particles true values"
	};
	vector<string> level2_descr = {
		"detected particles true values", 
		"detected particles reco values",
		"detected particles true values"
	};

	vector<string> level_titel = {
		"final elementon reco",
		"jet reconstruction",
		"detector acceptance"
	};

	vector<string> observables = {"Ejet", "pjet"};
	
	vector<int> level1_index = { 0, 2, 1};
	vector<int> level2_index = { 2, 3, 2};

	vector<vector<float**>> level_i_observables = { 
		{&TJ_jets_final_elementon_E, &TJ_jets_final_elementon_p},
		{&TJ_jets_true_E, &TJ_jets_true_p},
		{&TJ_jets_true_of_detectable_E, &TJ_jets_true_of_detectable_p},
		{&TJ_jets_detected_E, &TJ_jets_detected_p}
	};

	void define_plots(){

	  for (int ilevel=0; ilevel<levels.size(); ilevel++) {
		string level = levels[ilevel];

		for (int iobs=0; iobs<observables.size(); iobs++) {
		  string obs = observables[iobs];

		  for (int itype=0; itype<particle_types.size(); itype++) {
			string type = particle_types[itype];

			for (int iNclass=0; iNclass<Ngamma_classes.size(); iNclass++) {
				string Ngamma_class = Ngamma_classes[iNclass]; 

				for (int iclass=0; iclass<gamma_classes.size(); iclass++) {
					string gamma_class = gamma_classes[iclass];

					add_new_TProfile((	level +"_"+ obs +"_"+ type + "_jets_" + Ngamma_class +"_"+ 
										gamma_class + "_MCgammas").c_str(),
						new TProfile((	level +"_"+ obs +"_"+ type + "_jets_" + Ngamma_class +"_"+ 
										gamma_class + "_MCgammas").c_str(),
									(type + " " + level_titel[ilevel] + ", different Number " 
									+ gamma_class + " MCgammas;\
									E_{" + level1_descr[ilevel] + "}^{jet} [GeV]; \
									E_{" + level2_descr[ilevel] + "}^{jet} [GeV]").c_str(),
									50, 0, 150)
						);


				}
			}

			for (int iEclass=0; iEclass<gamma_E_classes.size(); iEclass++) {
				string Egamma_string = to_string(gamma_E_classes[iEclass]);
				add_new_TProfile((	level +"_"+ obs +"_"+ type+ "_jets_no_E_geq" + Egamma_string 
									+ "_MCgammas").c_str(),
					new TProfile((	level +"_"+ obs +"_"+ type+ "_jets_no_E_geq" + Egamma_string 
									+ "_MCgammas").c_str(),
								(type + " " + level_titel[ilevel] + ", only jets with no gammas_{E>Emax};\
								E_{" + level1_descr[ilevel] + "}^{jet} [GeV]; \
								E_{" + level2_descr[ilevel] + "}^{jet} [GeV]").c_str(),
								50, 0, 150) ); 

			}
		  }
		}
	  }
	}

	bool is_good_gamma( int ijet, int igamma ) {
		if (TJ_MCGamma_exists[ijet][igamma] == 0) { return false; }
		else if ( 	(TJ_MCGamma_highest_trck_weight_in_this_jet[ijet][igamma] < 0.5) &&
					(TJ_MCGamma_highest_calo_weight_in_this_jet[ijet][igamma] < 0.5) ) {
			return false; 
		}
		else { return true; }

	}

	void fill_plots(){
		float weight = get_current_weight();

		// This is the loop over all events
		while ( get_next_event() ) {
			if ( pass_selection == 0 ) { continue; } 

			for (int ijet=0; ijet<max_Njets; ijet++) {

				if ( ! TJ_jets_exists[ijet] ) { continue; } 

				if ( fabs(TJ_jets_final_elementon_pdgID[ijet]) > 5 ) { continue; }

				/* ------------------ */
				// Fixed E between high and low E gammas
				int N_highE_gammas = 0;
				int N_lowE_gammas = 0;
				for (int igamma=0; igamma<max_Nparticles; igamma++) {
					if ( ! is_good_gamma(ijet, igamma) ) { continue; }
					if ( TJ_MCGamma_E[ijet][igamma] > 5 ) { N_highE_gammas++; }
					else { N_lowE_gammas++; }
				}

				string N_highE_gammas_string = Ngamma_class_from_int( N_highE_gammas );
				string N_lowE_gammas_string = Ngamma_class_from_int( N_lowE_gammas );

				vector<string> N_current_classes = {
					N_highE_gammas_string,
					N_lowE_gammas_string
				};

				string type;

				if ( fabs(TJ_jets_final_elementon_pdgID[ijet]) == 5 ) {
					type = "b";
				} else {
					type = "udsc";
				}


				for (int iclass=0; iclass<gamma_classes.size(); iclass++) {
					string gamma_class = gamma_classes[iclass];
					string N_current_class = N_current_classes[iclass];
		
					for (int iobs=0; iobs<observables.size(); iobs++) {

						for (int ilevel=0; ilevel<levels.size(); ilevel++) {

							get_TProfile( (levels[ilevel] + "_" + observables[iobs]  
											+ "_all_q_jets_" + N_current_class + "_" 
											+ gamma_class + "_MCgammas").c_str() )
							->Fill( (*((level_i_observables[level1_index[ilevel]])[iobs]))[ijet], 
									(*((level_i_observables[level2_index[ilevel]])[iobs]))[ijet], 
									weight );

							get_TProfile( (levels[ilevel] + "_" + observables[iobs]  
											+ "_" + type + "_jets_" + N_current_class + "_" 
											+ gamma_class + "_MCgammas").c_str() )
							->Fill( (*((level_i_observables[level1_index[ilevel]])[iobs]))[ijet], 
									(*((level_i_observables[level2_index[ilevel]])[iobs]))[ijet], 
									weight );


						}
					}
				}

				/* ------------------ */


				/* ------------------ */
				// Investigating maximum allowed gamma E 
				
				for (int iEclass=0; iEclass<gamma_E_classes.size(); iEclass++) {
					float Egammamax = float(gamma_E_classes[iEclass]); 			
					int N_Egammas = 0;
					for (int igamma=0; igamma<max_Nparticles; igamma++) {
						if ( ! is_good_gamma( ijet, igamma) ) { continue; }
						if ( TJ_MCGamma_E[ijet][igamma] > Egammamax ) { N_Egammas++; }
					}

					if ( N_Egammas == 0 ) {
						string Egamma_string = to_string(gamma_E_classes[iEclass]);

						for (int iobs=0; iobs<observables.size(); iobs++) {
							for (int ilevel=0; ilevel<levels.size(); ilevel++) {

								get_TProfile( (	levels[ilevel] + "_" + observables[iobs] + 
											"_all_q_jets_no_E_geq" + Egamma_string + 
											"_MCgammas").c_str() )
								->Fill(
									(*((level_i_observables[level1_index[ilevel]])[iobs]))[ijet],
									(*((level_i_observables[level2_index[ilevel]])[iobs]))[ijet], 
									weight );


								get_TProfile( (	levels[ilevel] + "_" + observables[iobs] + 
											"_" + type + "_jets_no_E_geq" + Egamma_string + 
											"_MCgammas").c_str() )
								->Fill(
									(*((level_i_observables[level1_index[ilevel]])[iobs]))[ijet],
									(*((level_i_observables[level2_index[ilevel]])[iobs]))[ijet], 
									weight );

							}
						} 
					}
				}

				

			}
							

		}
	}


	void draw_plots(){
		string output_dir = get_output_directory();

		vector<array<float, 3>> colors {{27,158,119}, {217,95,2}, {117,112,179}, {231,41,138}, {102,166,30}, {230,171,2}};
		for (int j=0; j<colors.size(); j++) {
		    TColor *h2_color = new TColor(9100+j, colors[j][0]/256.0, colors[j][1]/256.0, colors[j][2]/256.0);
    	}                                                                                              

		for (int ilevel=0; ilevel<levels.size(); ilevel++) {
		  string level = levels[ilevel];

		  for (int iobs=0; iobs<observables.size(); iobs++) {
			string obs = observables[iobs];

			for (int itype=0; itype<particle_types.size(); itype++) {
		   	  string type = particle_types[itype];
			  for (int iclass=0; iclass<gamma_classes.size(); iclass++) {
				string gamma_class = gamma_classes[iclass];

				TCanvas* can = new TCanvas( ("can_" + type + "_" + gamma_class).c_str(), 
											"", 0, 0, 600, 600); 	
				TPad* pad_up 	= new TPad( ("p1_" + type + "_" + gamma_class).c_str(), 
											"", 0.,0.45,1.,1.,0); 	
				pad_up->Draw();

				TPad* pad_down 	= new TPad( ("p2_" + type + "_" + gamma_class).c_str(), 
											"", 0.,0.,1.,0.5005,0); 	
				pad_down->Draw();
				pad_down->SetTopMargin(0);

				THStack* profile_stack = new THStack(("can_" + type +"_"+ gamma_class).c_str(),
											(type +" "+ level_titel[ilevel] +", different Number "
											+ gamma_class + " MCgammas; ; \
											E_{" +level2_descr[ilevel]+ "}^{jet} [GeV]").c_str());

				THStack* prjctn_stack = new THStack(("proj_can_" + type +"_"+ gamma_class).c_str(),
											(";	E_{" +level1_descr[ilevel]+ "}^{jet} [GeV]; \
											Events").c_str());


				TLegend* leg = new TLegend(0.25, 0.6, 0.45, 0.9);
	
				for (int iNclass=0; iNclass<Ngamma_classes.size(); iNclass++) {
					string Ngamma_class = Ngamma_classes[iNclass]; 

					TProfile* profile = get_TProfile( ( level +"_"+ obs +"_"+ type + "_jets_" 
														+ Ngamma_class + "_" + gamma_class 
														+ "_MCgammas").c_str() );
					profile->SetLineColor(9100+iNclass);
					profile->SetMarkerColor(9100+iNclass);
					leg->AddEntry(profile, (Ngamma_class+"_{"+gamma_class_labels[iclass]+"}").c_str(),
								"p");
					profile_stack->Add(profile);

					TH1D* entries_h1 = new TH1D((string(profile->GetName()) + "_pX").c_str(),
						(string(" ;") + profile->GetXaxis()->GetTitle() + "; Events" ).c_str(),
						profile->GetNbinsX(), profile->GetXaxis()->GetXmin(),
						profile->GetXaxis()->GetXmax() );
					for (int ibin=0; ibin<profile->GetNbinsX(); ibin++) {
						entries_h1->SetBinContent(ibin+1, profile->GetBinEntries(ibin+1));
					}
					entries_h1->SetLineColor(9100+iNclass);
					entries_h1->SetMarkerColor(9100+iNclass);
					entries_h1->SetMarkerSize(0.1);
					prjctn_stack->Add(entries_h1);
				}

				pad_up->cd();
				profile_stack->Draw("nostack");
				gPad->GetFrame()->SetY1(0);
				gPad->Update();
				float axis_max = gPad->GetFrame()->GetY2();
				if ( axis_max > 150 ) { axis_max = 150; }
				TLine* profile_line = new TLine(0, 0, axis_max, axis_max);

				profile_line->SetLineWidth(1); 
				profile_line->Draw("same");
				leg->Draw();

				pad_down->cd();
				prjctn_stack->Draw("hist nostack");

				string plot_name_profile = output_dir + "/" + level +"/"+ obs +"_"+ type 
													+ "_jets_" + gamma_class + "_MCgammas.pdf";
				can->Print(plot_name_profile.c_str());
				can->Close();
				delete profile_line;
			  }


			TCanvas* can = new TCanvas( ("can_" + type + "_EgammaMax").c_str(),
										"", 0, 0, 600, 600);				
			TPad* pad_up 	= new TPad( ("p1_" + type + "_EgammaMax").c_str(), 
										"", 0.,0.45,1.,1.,0); 	
			pad_up->Draw();

			TPad* pad_down 	= new TPad( ("p2_" + type + "_EgammaMax").c_str(), 
										"", 0.,0.,1.,0.5005,0); 	
			pad_down->Draw();
			pad_down->SetTopMargin(0);

			THStack* profile_stack = new THStack(("can_" + type + "_EgammaMax").c_str(),
								(type+" "+level_titel[ilevel]+", only jets with no gammas_{E>Emax};\
								;E_{" + level2_descr[ilevel] + "}^{jet} [GeV]").c_str());

			THStack* prjctn_stack = new THStack(("proj_can_EgammaMax_" + type).c_str(),
										(";	E_{" + level1_descr[ilevel] + "}^{jet} [GeV]; \
										Events").c_str());

			TLegend* leg = new TLegend(0.25, 0.6, 0.45, 0.9);

			for (int iEclass=0; iEclass<gamma_E_classes.size(); iEclass++) {		
				string Egamma_string = to_string(gamma_E_classes[iEclass]);
				
				TProfile* profile = get_TProfile( 	(level +"_"+ obs +"_"+ type + "_jets_no_E_geq"
													+ Egamma_string + "_MCgammas").c_str() );	
				profile->SetLineColor(9100+iEclass);
				profile->SetMarkerColor(9100+iEclass);
				//profile->SetMarkerSize(0.1);
				leg->AddEntry(profile, ("No MCgamma_{E>" + Egamma_string + "}").c_str(), "p");
				profile_stack->Add(profile);

				TH1D* entries_h1 = new TH1D((string(profile->GetName()) + "_pX").c_str(),
					(string(" ;") + profile->GetXaxis()->GetTitle() + "; Events" ).c_str(),
					profile->GetNbinsX(), profile->GetXaxis()->GetXmin(),
					profile->GetXaxis()->GetXmax() );
				for (int ibin=0; ibin<profile->GetNbinsX(); ibin++) {
					entries_h1->SetBinContent(ibin+1, profile->GetBinEntries(ibin+1));
				}
				entries_h1->SetLineColor(9100+iEclass);
				entries_h1->SetMarkerColor(9100+iEclass);
				entries_h1->SetMarkerSize(0.1);
				prjctn_stack->Add(entries_h1);

			}

			pad_up->cd();
			profile_stack->Draw("nostack");
			gPad->GetFrame()->SetY1(0);
			gPad->Update();
			float axis_max = gPad->GetFrame()->GetY2();
			if ( axis_max > 150 ) { axis_max = 150; }
			TLine* profile_line_Emax = new TLine(0, 0, axis_max, axis_max);
			profile_line_Emax->SetLineWidth(1); 
			profile_line_Emax->Draw("same");
			leg->Draw();

			pad_down->cd();
			prjctn_stack->Draw("hist nostack");

			string plot_name_profile = output_dir + "/" + level +"/"+ obs +"_"+ type 
												+ "_jets_varying_Emax_MCgammas.pdf";
			can->Print(plot_name_profile.c_str());
			can->Close();
			delete profile_line_Emax;

		  }
		}
	  }		
	}
			
};
