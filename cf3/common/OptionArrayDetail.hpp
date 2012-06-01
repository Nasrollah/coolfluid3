// Copyright (C) 2010-2011 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#ifndef cf3_common_OptionArrayDetail_hpp
#define cf3_common_OptionArrayDetail_hpp

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include "common/BoostFilesystem.hpp"

#include "rapidxml/rapidxml.hpp"

#include "common/StringConversion.hpp"
#include "common/URI.hpp"
#include "common/BasicExceptions.hpp"

#include "common/XML/Map.hpp"
#include "common/XML/CastingFunctions.hpp"
#include "Core.hpp"
#include "Component.hpp"

class C;
namespace cf3 {
namespace common {

namespace detail
{
  // Handle the int <-> Uint conflicts
  /// Helper function to set the value
  template<typename TYPE>
  inline void change_array_value(boost::any& to_set, const boost::any& new_value)
  {
    cf3_assert(new_value.type() == to_set.type());
    to_set = new_value; // update the value
  }

  template<>
  inline void change_array_value<Uint>(boost::any& to_set, const boost::any& new_value)
  {
    if(new_value.type() == to_set.type())
    {
      to_set = new_value;
    }
    else
    {
      try
      {
        std::vector<int> int_vals = boost::any_cast< std::vector<int> >(new_value);
        const Uint nb_vals = int_vals.size();
        std::vector<Uint> result(nb_vals);
        for(Uint i = 0; i != nb_vals; ++i)
        {
          if(int_vals[i] < 0)
            throw BadValue(FromHere(), "Tried to store a negative value in an unsigned int option array at index " + boost::lexical_cast<std::string>(i));
          result[i] = static_cast<Uint>(int_vals[i]);
        }
        to_set = result;
      }
      catch(boost::bad_any_cast& e)
      {
        throw CastingFailed(FromHere(), std::string("Failed to cast object of type ") + new_value.type().name() + " to type std::vector<Uint>");
      }
    }
  }

  template<>
  inline void change_array_value<int>(boost::any& to_set, const boost::any& new_value)
  {
    if(new_value.type() == to_set.type())
    {
      to_set = new_value;
    }
    else
    {
      try
      {
        std::vector<Uint> int_vals = boost::any_cast< std::vector<Uint> >(new_value);
        const Uint nb_vals = int_vals.size();
        std::vector<int> result(nb_vals);
        for(Uint i = 0; i != nb_vals; ++i)
        {
          result[i] = static_cast<int>(int_vals[i]);
        }
        to_set = result;
      }
      catch(boost::bad_any_cast& e)
      {
        throw CastingFailed(FromHere(), std::string("Failed to cast object of type ") + new_value.type().name() + " to type std::vector<int>");
      }
    }
  }

  template<>
  inline void change_array_value<Real>(boost::any& to_set, const boost::any& new_value)
  {
    if(new_value.type() == to_set.type())
    {
      to_set = new_value;
    }
    else
    {
      try
      {
        std::vector<int> int_vals = boost::any_cast< std::vector<int> >(new_value);
        const Uint nb_vals = int_vals.size();
        std::vector<Real> result(nb_vals);
        for(Uint i = 0; i != nb_vals; ++i)
        {
          result[i] = static_cast<Real>(int_vals[i]);
        }
        to_set = result;
      }
      catch(boost::bad_any_cast& e)
      {
        throw CastingFailed(FromHere(), std::string("Failed to cast object of type ") + new_value.type().name() + " to type std::vector<Real>");
      }
    }
  }

  /// A struct, since partial specializations of functions is not allowed
  template<typename TYPE>
  struct ArrayToVector
  {
    inline std::vector<TYPE> operator()(const XML::XmlNode& node) const
    {
      return XML::Map().array_to_vector<TYPE>( node );
    }
  };

  template<typename ComponentT>
  struct ArrayToVector< Handle<ComponentT> >
  {
    typedef Handle<ComponentT> TYPE;
    inline std::vector<TYPE> operator()(const XML::XmlNode& node) const
    {
      const std::vector<URI> uri_vect = XML::Map().array_to_vector<URI>( node );
      std::vector<TYPE> result; result.reserve(uri_vect.size());
      BOOST_FOREACH(const URI& uri, uri_vect)
      {
        result.push_back(TYPE(Core::instance().root().access_component(uri)));
      }

      return result;
    }
  };

  /// A struct, since partial specializations of functions is not allowed
  template<typename TYPE>
  struct FromString
  {
    inline TYPE operator()(const std::string& str) const
    {
      return from_str<TYPE>(str);
    }
  };

  template<typename ComponentT>
  struct FromString< Handle<ComponentT> >
  {
    typedef Handle<ComponentT> TYPE;
    inline TYPE operator()(const std::string& str) const
    {
      const URI uri = from_str<URI>(str);
      return TYPE(Core::instance().root().access_component(uri));
    }
  };

} // detail
} // common
} // cf3

#endif // cf3_common_OptionArrayDetail_hpp
