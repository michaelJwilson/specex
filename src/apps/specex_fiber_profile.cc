#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <map>

#include <boost/program_options.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>

#include <harp.hpp>

#include <specex_message.h>
#include <specex_psf.h>
#include <specex_trace.h>
#include <specex_spot.h>
#include <specex_spot_array.h>
#include <specex_fits.h>
#include <specex_gauss_hermite_psf.h>
#include <specex_serialization.h>
#include <specex_image_data.h>
#include <specex_stamp.h>
#include <specex_serialization_implement.h>



using namespace std;
using namespace specex;

namespace popts = boost::program_options;




int main ( int argc, char *argv[] ) {
  

  

  // default arguments
  // --------------------------------------------
  string spectrograph_name = "BOSS";
  int    fiber=1;
  string psf_xml_filename="";
  string spots_xml_filename="";
  string input_fits_image_filename="";
  string output_filename="";  
  double psf_error=0;
  double readout_noise=2;
  
  // reading arguments
  // --------------------------------------------
  popts::options_description desc ( "Allowed Options" );
  desc.add_options()
    ( "help,h", "display usage information" )
    ( "psf", popts::value<string>( &psf_xml_filename ), "psf xml filename" )
    ( "spots", popts::value<string>( &spots_xml_filename ), "spots xml filename" )
    ( "fiber", popts::value<int>( &fiber ), "" )
    ( "in", popts::value<string>( &input_fits_image_filename ), " input fits image file name" )
    ( "out", popts::value<string>( &output_filename ), " output file name")
    ( "readout-noise", popts::value<double>( &readout_noise ), " readout noise for pull")
    ( "psf-error", popts::value<double>( &psf_error ), " psf relative error for pull")
    ( "verbose,v", "turn on verbose mode" )
    ( "core", "dump core files when harp exception is thrown" )
    ;
  
  popts::variables_map vm;
  
  try {
  
    
    popts::store(popts::command_line_parser( argc, argv ).options(desc).run(), vm);
    popts::notify(vm);
    
    if ( ( argc < 2 ) || vm.count( "help" ) || ( ! vm.count( "psf" ) )  || ( ! vm.count( "spots" ) ) 
	 || ( ! vm.count( "in" ) ) || ( ! vm.count( "fiber" ) ) || ( ! vm.count( "out" ) )  ) {
      cerr << endl;
      cerr << desc << endl;
      cerr << "example:" << endl;
      cerr << argv[0] << " --psf psf.xml  --spots spots.xml --in sdProc-b1-00108382.fits --fiber 20 --out res.list" << endl;
      return EXIT_FAILURE;
    }
  }catch(std::exception) {
    cerr << "error in arguments" << endl;
    cerr << endl;
    cerr << desc << endl;
    return EXIT_FAILURE;
  }
  //try {

    specex_set_verbose(vm.count("verbose")>0);
    specex_set_dump_core(vm.count("core")>0);
    
  

    
    
    // open image
    // --------------------------------------------
    image_data image,weight;
    read_fits_images(input_fits_image_filename,image,weight);
    // weight is 0 or 1
    for(int j=0;j<weight.Ny();j++) {
      for(int i=0;i<weight.Nx();i++) {
	if(weight(i,j)>0)
	  weight(i,j)=1;
	else
	  weight(i,j)=0;
      }
    }
    
    // read PSF
    // --------------------------------------------
    specex::PSF_p psf;
    {
      std::ifstream is(psf_xml_filename.c_str());
      boost::archive::xml_iarchive xml_ia ( is );
      xml_ia >> BOOST_SERIALIZATION_NVP(psf);
      is.close();
    }
    
    
    // read spots
    // --------------------------------------------
    vector<specex::Spot_p> input_spots;
    {
      std::ifstream is(spots_xml_filename.c_str());
      boost::archive::xml_iarchive xml_ia ( is );
      xml_ia >> BOOST_SERIALIZATION_NVP(input_spots);
      is.close();
    } 
    SPECEX_INFO("fiber = " << fiber);

    SPECEX_INFO("number of input spots = " << input_spots.size());

    vector<specex::Spot_p> spots;
    for(size_t s=0;s<input_spots.size();s++) {
      specex::Spot_p& spot = input_spots[s];
      // if(spot->fiber != fiber) continue; // keep all spots of course !!
      if(spot->status == 0) continue;
      spots.push_back(spot);
    }
    
    SPECEX_INFO("number of selected spots = " << spots.size());
    
    // create output images
    // --------------------------------------------
    
    specex::Trace& trace = psf->FiberTraces[fiber];
    double wmin = trace.X_vs_W.xmin;
    double wmax = trace.X_vs_W.xmax;
    double ymin = max(0,int(psf->Yccd(fiber,wmin)));
    double ymax = min(int(image.n_rows())-1,int(psf->Yccd(fiber,wmax)));
    
    SPECEX_INFO("wavelength range " << wmin << " " << wmax);
    SPECEX_INFO("y ccd      range " << ymin << " " << ymax);
    
    int hx = 3;
    ofstream os(output_filename.c_str());
    os << "# y :" << endl;
    os << "# w :" << endl;
    os << "# data :" << endl;
    os << "# model : core+tail+cont" << endl;
    os << "# core :" << endl;
    os << "# tail :" << endl;
    os << "# cont :" << endl;
    
    for(int p=0;p<psf->LocalNAllPar();p++) {
      os << "# " << psf->ParamName(p) << " :" << endl;
    }
    os << "#end" << endl;
    
    vector<harp::vector_double> psf_params;
    vector<harp::vector_double> psf_params_wo_tails;
    int tail_amp_index = psf->ParamIndex("TAILAMP");
    for(size_t s=0;s<spots.size();s++) {
      specex::Spot_p& spot = spots[s];
      if(spot->fiber != fiber) continue;
      psf_params.push_back(psf->AllLocalParamsFW(spot->fiber,spot->wavelength));
      harp::vector_double p = psf_params.back();
      p(tail_amp_index)=0;
      psf_params_wo_tails.push_back(p);
    }

    
#ifdef CONTINUUM
    int bundle=psf->GetBundleOfFiber(fiber);
    PSF_Params &params_of_bundle = psf->ParamsOfBundles.find(bundle)->second;
    double expfact_for_continuum=1./(sqrt(2*M_PI)*params_of_bundle.continuum_sigma_x);
#endif
    
    

    for(int j=int(ymin) ; j<=int(ymax) ; j++) {
      
      if((j%100)==0) cout << "done " << j << endl;

      // get wave
      double wave = trace.W_vs_Y.Value(j);
      os << j << " " << wave;
      double x_center = trace.X_vs_W.Value(wave);
      int i_begin = int(x_center+0.5)-hx;
      int i_end   = i_begin + (2*hx+1);
    
      double data_j = 0;
      double val_j   = 0;
      double val_j_core   = 0;
      double val_j_tail   = 0;
      double val_j_cont   = 0;
      
      for(int i=i_begin ; i<i_end; i++) {
	
	if(weight(i,j)=0) continue;
	data_j += image(i,j);
	
	
	
#ifdef CONTINUUM
	double continuum_val = params_of_bundle.ContinuumPol.Value(wave)*expfact_for_continuum*exp(-0.5*square((i-x_center)/params_of_bundle.continuum_sigma_x));
	val_j += continuum_val;
	val_j_cont += continuum_val;
#endif


	for(size_t s=0;s<spots.size();s++) {
	  specex::Spot_p& spot = spots[s];
	  if(spot->fiber != fiber) continue;
	  
	  bool in_core = fabs(i-spot->xc)<2*psf->hSizeX && fabs(j-spot->yc)<2*psf->hSizeY;
	  double val = spot->flux*psf->PSFValueWithParamsXY(spot->xc, spot->yc,i, j,
							    psf_params[s], 0, 0, in_core , true);
	  
	  double val_wo_tail = spot->flux*psf->PSFValueWithParamsXY(spot->xc, spot->yc,i, j,
								    psf_params[s], 0, 0, in_core , false);
	  val_j += val;
	  val_j_core += val_wo_tail;
	  val_j_tail += (val-val_wo_tail);
	  
	}
	
      }
      
      os << " " << data_j << " " << val_j << " " << val_j_core << " " << val_j_tail << " " << val_j_cont;
      
      harp::vector_double params = psf->AllLocalParamsFW(fiber,wave);
      for(size_t p=0;p<params.size();p++)
	os << " " << params(p);
      os << endl;

    }
    os.close();
    


  // ending
  // --------------------------------------------


    //}catch(harp::exception e) {
    //cerr << "FATAL ERROR (harp) " << e.what() << endl;
    //return EXIT_FAILURE;
    //}

    
    /*
      }catch(std::exception e) {
    cerr << "FATAL ERROR " << e.what() << endl;
    return EXIT_FAILURE;
    }*/

  return EXIT_SUCCESS;
}



