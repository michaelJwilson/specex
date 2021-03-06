
#include <harp_plugin_specex.h>


using namespace std;

// see HARP/src/libharp/data/harp/psf.hpp
// see HARP/src/libharp/plugin/harp_plugin_image_fits.cpp

harp::specex_psf::specex_psf ( boost::property_tree::ptree const & props ) : harp::psf ( "specex", props ) {

  // read specex psf in xml format
  
  cout << "INFO harp::specex_psf Reading a specex::PSF" << endl;
  
  string file_type = props.get("type","");
  string path = props.get("path","");
  
  if( file_type == "xml")
    read_psf_xml(actual_specex_psf,path);
  else if( file_type == "fits")
    read_psf_fits(actual_specex_psf,path);
  
  double wavebin = props.get("wavebin",1.);
  double wavemin = props.get("wavemin",0.);
  double wavemax = props.get("wavemax",0.);
  fibermin_ = props.get("fibermin",0);
  fibermax_ = props.get("fibermax",actual_specex_psf->FiberTraces.size()-1);
  interpolation_ = (props.get("interpolation",0)==1);
  

  cout << "INFO " << wavebin << " " << wavemin << " " << wavemax << " " << fibermin_ << " " << fibermax_ << " interpolate = " << interpolation_ << endl;
  

  if(wavebin<=0) {
    HARP_THROW("unphysical wave bin");
  }

  

  {
    // compute range of wavelength accessible for all fibers as defined by the measured arc lamp lines
    if(wavemin==0) {
      wavemin=0;
      for(std::map<int,specex::Trace>::const_iterator it = actual_specex_psf->FiberTraces.begin(); it != actual_specex_psf->FiberTraces.end(); ++it) {
	wavemin=max(wavemin,it->second.Y_vs_W.xmin);
      }
    }
    if(wavemax==0) {
      wavemax=100000;
      for(std::map<int,specex::Trace>::const_iterator it = actual_specex_psf->FiberTraces.begin(); it != actual_specex_psf->FiberTraces.end(); ++it) {
	wavemax=min(wavemax,it->second.Y_vs_W.xmax);
      }
    }

    if(wavemin>=wavemax) HARP_THROW("error wavemin >= wavemax");
    
    int nlambda = int((wavemax-wavemin)/wavebin)+1; // included
    lambda_.resize(nlambda);
    for(int i=0;i<nlambda;i++)
      lambda_(i)=wavemin+wavebin*i;
  }
  cout << "INFO harp::specex_psf # lambda : " << lambda_.size() << " range : " << wavemin << " " << wavemax << endl;
  
  rows_  = actual_specex_psf->ccd_image_n_rows;
  cols_  = actual_specex_psf->ccd_image_n_cols;
  
  // create map of bundles
  bundle_.clear();
  for(std::map<int,specex::PSF_Params>::iterator bundle_it = actual_specex_psf->ParamsOfBundles.begin(); bundle_it != actual_specex_psf->ParamsOfBundles.end(); bundle_it++) {
    const specex::PSF_Params& params_of_bundle = bundle_it->second;
    for(int fiber=params_of_bundle.fiber_min; fiber<=params_of_bundle.fiber_max; fiber++) {
      bundle_[fiber]=params_of_bundle.bundle_id;
    }
  }
  

  cout << "INFO harp::specex_psf This PSF is a " << actual_specex_psf->Name() << endl;
}

void harp::specex_psf::extent ( size_t spec_index, size_t lambda_index, size_t & x_offset, size_t & y_offset, size_t & n_x, size_t & n_y ) const {
  
  int fiber = fibermin_+spec_index; 
  
  std::map<int,specex::Trace>::const_iterator it = actual_specex_psf->FiberTraces.find(fiber);
  if(it==actual_specex_psf->FiberTraces.end()) HARP_THROW("specex_psf::response don't have PSF for this fiber/spec");
  if(lambda_index<0 || lambda_index>=lambda_.size()) HARP_THROW("specex_psf::response invalid lambda index");
  const double &wave = lambda_(lambda_index);
  
  std::map<int,int>::const_iterator bundle_it = bundle_.find(fiber);
  if( bundle_it == bundle_.end()) HARP_THROW("specex_psf::response cannot find bundle of fiber");
  
  const specex::Trace &trace = it->second;
  double x_center = trace.X_vs_W.Value(wave);
  double y_center = trace.Y_vs_W.Value(wave);
  int x_pix_begin = int(floor(x_center))-actual_specex_psf->hSizeX;
  int y_pix_begin = int(floor(y_center))-actual_specex_psf->hSizeY;
  x_offset = x_pix_begin;
  y_offset = y_pix_begin;
  n_x=2*actual_specex_psf->hSizeX+1;
  n_y=2*actual_specex_psf->hSizeY+1; 
  return;
}

#define MINVAL 1.e-20

void harp::specex_psf::response ( size_t spec_index, size_t lambda_index, size_t & x_offset, size_t & y_offset, harp::matrix_double & patch ) const {
  
  int fiber = fibermin_+spec_index; 
  
  std::map<int,specex::Trace>::const_iterator it = actual_specex_psf->FiberTraces.find(fiber);
  if(it==actual_specex_psf->FiberTraces.end()) HARP_THROW("specex_psf::response don't have PSF for this fiber/spec");
  if(lambda_index<0 || lambda_index>=lambda_.size()) HARP_THROW("specex_psf::response invalid lambda index");
  const double &wave = lambda_(lambda_index);
  
  std::map<int,int>::const_iterator bundle_it = bundle_.find(fiber);
  if( bundle_it == bundle_.end()) HARP_THROW("specex_psf::response cannot find bundle of fiber");
  
  const specex::Trace &trace = it->second;
  double x_center = trace.X_vs_W.Value(wave);
  double y_center = trace.Y_vs_W.Value(wave);

  
  
  harp::vector_double params = actual_specex_psf->AllLocalParamsFW(fiber,wave,bundle_it->second);
  
  int nx=2*actual_specex_psf->hSizeX+1;
  int ny=2*actual_specex_psf->hSizeY+1;
  int x_pix_begin = int(floor(x_center))-actual_specex_psf->hSizeX;
  int y_pix_begin = int(floor(y_center))-actual_specex_psf->hSizeY;
  x_offset = x_pix_begin;
  y_offset = y_pix_begin;

#define FLIPPED
#ifdef FLIPPED
  patch.resize(ny,nx);  
#else
  patch.resize(nx,ny);
#endif

  patch.clear();
  
  if(!interpolation_) {

    double wavestep=0;
    if(lambda_index==0) 
      wavestep=lambda_(1)-lambda_(0);
    else if (lambda_index==(int(lambda_.size())-1))
      wavestep=lambda_(lambda_index)-lambda_(lambda_index-1);
    else
      wavestep=0.5*(lambda_(lambda_index+1)-lambda_(lambda_index-1));
    
    double sum = 0;
    for(int j=0;j<ny;j++) {
      for(int i=0;i<nx;i++) {
	double val = actual_specex_psf->PSFValueWithParamsXY(x_center,y_center,x_pix_begin+i,y_pix_begin+j,params,0,0,true,true);
	if(val<MINVAL) val=MINVAL;
#ifdef FLIPPED
	patch(j,i) = val;
#else
  	patch(i,j) = val;
#endif
	sum += val;
      }
    }
    if(sum<=0) HARP_THROW("specex_psf::response sum is <=0");
    patch *= (1./sum)*wavestep;
    return;
  }
  
  double wave_step=0.2; //
  double sum = 0;
  double wanted_sum = 0;
  
  // interpolation 
  if(lambda_index>0) {
    const double &wave1 = lambda_(lambda_index-1);
    // do sum over [wave1,wave] weighted by linear weight from 0 to 1
    
    wanted_sum += 0.5*(wave-wave1);
    
    for(double w=wave;w>=wave1;w -= wave_step) { // start from wave
      double weight = (w-wave1)/(wave-wave1);
      
      x_center = trace.X_vs_W.Value(w);
      y_center = trace.Y_vs_W.Value(w);

      for(int j=0;j<ny;j++) {
	for(int i=0;i<nx;i++) {
	  double val = actual_specex_psf->PSFValueWithParamsXY(x_center,y_center,x_pix_begin+i,y_pix_begin+j,params,0,0,true,true);
	  if(val<0) val=0;
#ifdef FLIPPED
	patch(j,i) += weight*val; 
#else
  	patch(i,j) += weight*val; 
#endif
	  sum += weight*val;
	}
      }
      
    }
     
  }
  if(lambda_index<int(lambda_.size())-1) {
    const double &wave2 = lambda_(lambda_index+1);
    // do sum over [wave,wave2] weighted by linear weight from 1 to 0
    
    wanted_sum += 0.5*(wave2-wave);
    
    for(double w=wave;w<=wave2;w += wave_step) { // start from wave
      double weight = (wave2-w)/(wave2-wave);
      
      x_center = trace.X_vs_W.Value(w);
      y_center = trace.Y_vs_W.Value(w);

      for(int j=0;j<ny;j++) {
	for(int i=0;i<nx;i++) {
	  double val = actual_specex_psf->PSFValueWithParamsXY(x_center,y_center,x_pix_begin+i,y_pix_begin+j,params,0,0,true,true);
	  if(val<0) val=0;
#ifdef FLIPPED
	patch(j,i) += weight*val; 
#else
  	patch(i,j) += weight*val;
#endif
	  sum += weight*val;
	}
      }
      
    }
  }
  
  if(sum<=0) HARP_THROW("specex_psf::response sum is <=0");
  patch *= (wanted_sum/sum);
  

}


size_t harp::specex_psf::response_nnz_estimate ( ) const {
  return (2*actual_specex_psf->hSizeX+1)*(2*actual_specex_psf->hSizeY);
}


// Define the plugin creation function here.

harp::psf * harp::specex_psf_create ( boost::property_tree::ptree const & props ) {
  return new harp::specex_psf ( props );
}


// Define the initialize function that will be called by the plugin registry.  There must
// be only one such function per *.so file, but you can register multiple plugins within
// this function call.

void initialize ( void * registry ) {

  harp::plugin_registry * reg = static_cast < harp::plugin_registry * > ( registry );

  string const & version = harp::source_version();

  // register psf plugin
  reg->register_psf ( "specex", harp::specex_psf_create, version );
  
  return;
}

