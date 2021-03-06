#ifndef SPECEX_LEGENDRE__H
#define SPECEX_LEGENDRE__H

#include <string>

#include <harp.hpp>

using namespace std;
 
namespace specex {
class Legendre1DPol
{

friend class boost::serialization::access;
 
 public :
 string name;
  harp::vector_double coeff;
  int deg;
  double xmin,xmax;
  
  Legendre1DPol(int i_deg=0, const double& i_xmin=0, const double& i_xmax=0);
  
  harp::vector_double Monomials(const double &x) const;
  double Value(const double &x) const;
  
  bool Fit(const harp::vector_double& x, const harp::vector_double& y, const harp::vector_double* ey=0, bool set_range = true);
  Legendre1DPol Invert(int add_degree=0) const;
  
  private :

    template < class Archive >
      void serialize ( Archive & ar, const unsigned int version ) {
      ar & BOOST_SERIALIZATION_NVP(name);
      ar & BOOST_SERIALIZATION_NVP(coeff);
      ar & BOOST_SERIALIZATION_NVP(deg);
      ar & BOOST_SERIALIZATION_NVP(xmin);
      ar & BOOST_SERIALIZATION_NVP(xmax);
      return;
    }

};
BOOST_SERIALIZATION_SHARED_PTR(Legendre1DPol)
typedef boost::shared_ptr < specex::Legendre1DPol > Legendre1DPol_p;
  
Legendre1DPol composed_pol(const Legendre1DPol& pol1, const Legendre1DPol& pol2);
 

class Legendre2DPol
{

  friend class boost::serialization::access;

 public :
  string name;
  harp::vector_double coeff;
  int xdeg,ydeg;
  double xmin,xmax,ymin,ymax;
  
 Legendre2DPol(int i_xdeg=0, const double& i_xmin=0, const double& i_xmax=0, 
	       int i_ydeg=0, const double& i_ymin=0, const double& i_ymax=0);
 
  harp::vector_double Monomials(const double &x,const double &y) const;
  double Value(const double &x,const double &y) const;
  void Fill();
  
  private :

    template < class Archive >
      void serialize ( Archive & ar, const unsigned int version ) {
      ar & BOOST_SERIALIZATION_NVP(name);
      ar & BOOST_SERIALIZATION_NVP(coeff);
      ar & BOOST_SERIALIZATION_NVP(xdeg);
      ar & BOOST_SERIALIZATION_NVP(ydeg);
      ar & BOOST_SERIALIZATION_NVP(xmin);
      ar & BOOST_SERIALIZATION_NVP(xmax);
      ar & BOOST_SERIALIZATION_NVP(ymin);
      ar & BOOST_SERIALIZATION_NVP(ymax);
      return;
    }

};
BOOST_SERIALIZATION_SHARED_PTR(Legendre2DPol)



class SparseLegendre2DPol
{

  friend class boost::serialization::access;

 protected :
  std::vector<int> non_zero_indices;
  
 public :
  string name;
  harp::vector_double coeff;
  int xdeg,ydeg;
  double xmin,xmax,ymin,ymax;
  int Npar() const { return non_zero_indices.size();}
  void Add(int i,int j);
  void Fill(bool sparse = true); // this is equivalent to a std Legendre2DPol is sparse=false
  void Clear(); // reset

  SparseLegendre2DPol(int i_xdeg=0, const double& i_xmin=0, const double& i_xmax=0, 
		      int i_ydeg=0, const double& i_ymin=0, const double& i_ymax=0);
  
  harp::vector_double Monomials(const double &x,const double &y) const;
  double Value(const double &x,const double &y) const;
  
  private :

    template < class Archive >
      void serialize ( Archive & ar, const unsigned int version ) {
      ar & BOOST_SERIALIZATION_NVP(name);
      ar & BOOST_SERIALIZATION_NVP(coeff);
      ar & BOOST_SERIALIZATION_NVP(xdeg);
      ar & BOOST_SERIALIZATION_NVP(ydeg);
      ar & BOOST_SERIALIZATION_NVP(xmin);
      ar & BOOST_SERIALIZATION_NVP(xmax);
      ar & BOOST_SERIALIZATION_NVP(ymin);
      ar & BOOST_SERIALIZATION_NVP(ymax);
      ar & BOOST_SERIALIZATION_NVP(non_zero_indices);
      return;
    }

};
BOOST_SERIALIZATION_SHARED_PTR(SparseLegendre2DPol)

// this if for convenience in the rest of the code
typedef specex::SparseLegendre2DPol Pol;
typedef boost::shared_ptr < specex::SparseLegendre2DPol > Pol_p;

}
  
#endif /* SPECEX_LEGENDRE__H */
