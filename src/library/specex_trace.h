#ifndef SPECEX_TRACE__H
#define SPECEX_TRACE__H



#include <vector>
#include <string>

#include "specex_legendre.h"


namespace specex {
  class Spot;
  

#define SPECEX_TRACE_DEFAULT_LEGENDRE_POL_DEGREE 6
  
  class Trace {
    
    friend class boost::serialization::access;
    
  protected :
    
    
  public :
    
    int fiber; // this trace is for this fiber
    Legendre1DPol X_vs_lW; // maps x ccd coordinate as a function of log10(wavelength)
    Legendre1DPol Y_vs_lW; // maps y ccd coordinate as a function of log10(wavelength)
    
    Legendre1DPol lW_vs_Y; // as saved is SDSS fits file
    Legendre1DPol X_vs_Y; // as saved is SDSS fits file
    
    // example in /clusterfs/riemann/raid006/bosswork/boss/spectro/redux/current/4097/spArc-r1-00121688.fits.gz[2]
    double yjumplo; // KEY  XJUMPLO in fits table
    double yjumphi; // KEY  XJUMPHI in fits table
    double yjumpval; // KEY  XJUMPVAL in fits table
    
    
    Trace(int i_fiber=-1);
    
    bool Fit(std::vector<Spot*> spots, bool set_xy_range = true);
    
    /*
      void write(std::ostream &os) const;
      bool read(std::istream &is);
      void write(const std::string &FileName) const;
      bool read(const std::string &FileName) ;
    */
    
    private :

    template < class Archive >
      void serialize ( Archive & ar, const unsigned int version ) {
      ar & BOOST_SERIALIZATION_NVP(fiber);
      ar & BOOST_SERIALIZATION_NVP(X_vs_lW);
      ar & BOOST_SERIALIZATION_NVP(Y_vs_lW);
      ar & BOOST_SERIALIZATION_NVP(lW_vs_Y);
      ar & BOOST_SERIALIZATION_NVP(X_vs_Y);
      ar & BOOST_SERIALIZATION_NVP(yjumplo);
      ar & BOOST_SERIALIZATION_NVP(yjumphi);
      ar & BOOST_SERIALIZATION_NVP(yjumpval);
      
        return;
    }

    BOOST_SERIALIZATION_SHARED_PTR(Trace)  
  };
  
  // SDSS IO
  // must be replaced by reading dimension in traceset
#define NUMBER_OF_FIBERS_PER_CCD 500
#define NUMBER_OF_FIBERS_PER_BUNDLE 20
  
  enum TraceSetType {WY = 0, XY = 1, YW = 2, XW = 3};
 
  class TraceSet : public std::vector<Trace> {

    friend class boost::serialization::access;

  private :
    
    
    
  public :
    TraceSet() {};
    ~TraceSet(){};
    
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
      ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(std::vector<Trace>);
    }
  };
  
}
#endif
