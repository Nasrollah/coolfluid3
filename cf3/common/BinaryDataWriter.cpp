// Copyright (C) 2010-2013 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/assign/list_of.hpp>

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/zlib.hpp>

#include "common/Signal.hpp"
#include "common/PropertyList.hpp"
#include "common/OptionList.hpp"
#include "common/BinaryDataWriter.hpp"
#include "common/FindComponents.hpp"

#include "common/PE/Comm.hpp"

#include "common/XML/FileOperations.hpp"
#include "common/XML/XmlNode.hpp"

#include "common/LibCommon.hpp"

namespace cf3 {
namespace common {

struct BinaryDataWriter::Implementation
{
  Implementation(const URI& file) :
    filename(build_filename(file, PE::Comm::instance().rank())),
    xml_filename(file),
    out_file(filename, std::ios_base::out | std::ios_base::binary),
    index(0),
    xml_doc("1.0", "ISO-8859-1")
  {
    const Uint v = version();
    out_file.write(reinterpret_cast<const char*>(&v), sizeof(Uint));

    PE::Comm& comm = PE::Comm::instance();
    // Rank 0 writes out an XML file that lists all filenames for all CPUs
    if(comm.rank() == 0)
    {
      XmlNode cfbinary = xml_doc.add_node("cfbinary");
      cfbinary.set_attribute("version", to_str(version()));
      node_xml_data.reserve(comm.size());
      XmlNode node_list = cfbinary.add_node("nodes");
      for(Uint i = 0; i != comm.size(); ++i)
      {
        XmlNode node = node_list.add_node("node");
        node.set_attribute("filename", build_filename(file, i));
        node.set_attribute("rank", to_str(i));
        node_xml_data.push_back(node);
      }
    }
  }

  ~Implementation()
  {
    out_file.close();
    if(PE::Comm::instance().rank() == 0)
      XML::to_file(xml_doc, xml_filename);
  }

  Uint write_data_block(const char* data, const std::streamsize count, const std::string& list_name, const Uint nb_rows, const Uint nb_cols)
  {
    PE::Comm& comm = PE::Comm::instance();
    // Prefix and suffix markers
    static const std::string block_prefix("__CFDATA_BEGIN");
    static const std::string block_suffix("__CFDATA_END");

    const Uint block_begin = out_file.tellp();

    // Build a compressed stream
    std::stringstream raw_data(std::ios_base::in | std::ios_base::out | std::ios_base::binary);
    boost::iostreams::filtering_ostream out_block;
    out_block.push(boost::iostreams::zlib_compressor());
    out_block.push(raw_data);
    out_block.write(data, count);
    out_block.flush();

    out_file << block_prefix << raw_data.rdbuf() << block_suffix;

    const Uint block_end = out_file.tellp();

    // Data describing the block on the current CPU
    const std::vector<Uint> my_block_info = boost::assign::list_of(nb_rows)(nb_cols)(block_begin)(block_end);
    const Uint block_info_size = my_block_info.size();
    std::vector<Uint> global_block_info;
    const Uint root = 0;
    if(comm.is_active())
    {
      comm.gather(my_block_info, global_block_info, root);
    }
    else
    {
      global_block_info = my_block_info;
    }

    // Write out the XML data
    if(comm.rank() == root)
    {
      const Uint nb_procs = comm.size();
      for(Uint i = 0; i != nb_procs; ++i)
      {
        XmlNode block_xml = node_xml_data[i].add_node("block");
        const Uint j = i*block_info_size;
        block_xml.set_attribute("name", list_name);
        block_xml.set_attribute("index", to_str(index));
        block_xml.set_attribute("nb_rows", to_str(global_block_info[j]));
        block_xml.set_attribute("nb_cols", to_str(global_block_info[j+1]));
        block_xml.set_attribute("begin", to_str(global_block_info[j+2]));
        block_xml.set_attribute("end", to_str(global_block_info[j+3]));
      }
    }

    ++index;

    return index - 1;
  }

  Uint version() const
  {
    static const Uint current_version = 1;
    return current_version;
  }

  std::string build_filename(const URI& input, const Uint rank)
  {
    const URI my_dir = input.base_path();
    const std::string basename = input.base_name();
    const URI result(my_dir / (basename + "_P" + to_str(rank) + ".cfbin"));
    return result.path();
  }

  const std::string filename;
  const URI xml_filename;
  boost::filesystem::fstream out_file;

  // Index of the next block to write
  Uint index;

  // XML document describing all data added (only valid on root process)
  XmlDoc xml_doc;

  std::vector<XmlNode> node_xml_data;
};
  
////////////////////////////////////////////////////////////////////////////////////////////

BinaryDataWriter::BinaryDataWriter ( const std::string& name ) : Component(name)
{
  options().add("file", URI())
    .pretty_name("File")
    .description("File name for the output file")
    .attach_trigger(boost::bind(&BinaryDataWriter::trigger_file, this));
}

BinaryDataWriter::~BinaryDataWriter()
{
  close();
}

void BinaryDataWriter::close()
{
  m_implementation.reset();
}

std::string BinaryDataWriter::file_name() const
{
  if(is_null(m_implementation.get()))
    throw SetupError(FromHere(), "BinaryDataWriter has no active file");
  return m_implementation->filename;
}

Uint BinaryDataWriter::write_data_block(const char* data, const std::streamsize count, const std::string& list_name, const Uint nb_rows, const Uint nb_cols)
{
  if(is_null(m_implementation.get()))
  {
    m_implementation.reset(new Implementation(options().value<URI>("file")));
  }

  return m_implementation->write_data_block(data, count, list_name, nb_rows, nb_cols);
}

void BinaryDataWriter::trigger_file()
{
  m_implementation.reset();
}

////////////////////////////////////////////////////////////////////////////////////////////

} // common
} // cf3
