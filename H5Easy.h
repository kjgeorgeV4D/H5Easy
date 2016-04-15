/*
 * Class functions for an easier h5 read and write method.
 * Created by: Steven Walton
 * Email: walton.stevenj@gmail.com
 * Version: 0.10
 */
#ifndef H5RW_H
#define H5RW_H

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <typeinfo>

#include "H5Cpp.h"

using namespace H5;

class WriteH5
{
   private:
   public:
      std::string variable;
      std::string filename;
      // sets our filename and our variable name
      void setFileName ( std::string name ) {filename = name;};
      void setVarName  ( std::string name ) {variable = name;};
      // Functions to be overloaded
      template<typename T>
      void writeData(const std::vector<T>);

      void createGroup(std::string);
};

class LoadH5
{
   private:
   public:
      std::string variable;
      std::string filename;
      // sets our filename and our variable name
      void setFileName(std::string name) {filename = name;};
      void setVarName(std::string name) {variable = name;};
      // Read functions
      std::vector<int> getDataint() const;
      std::vector<float> getDatafloat() const;
      std::vector<double> getDatadouble() const;
      // We now make a proxy class so that we can overload the return type and use a single
      // function to get data whether int or float. This could be made more advanced by 
      // adding more data types (such as double). 
      class Proxy
      {
         private:
            LoadH5 const* myOwner;
         public:
            Proxy( const LoadH5* owner ) : myOwner( owner ) {}
            operator std::vector<int>() const
            {
               return myOwner->getDataint();
            }
            operator std::vector<float>() const
            {
               return myOwner->getDatafloat();
            }
            operator std::vector<double>() const
            {
               return myOwner->getDatadouble();
            }
      };
      // Here we use the Proxy class to have a single getData function
      Proxy getData() const {return Proxy(this);}
};


#endif
/*
 * Source file for the easy implementation of h5 read and writing
 * Created by: Steven Walton
 * Email: walton.stevenj@gmail.com
 */
//#include "h5rw.h"

/*
 * ************************************************************************************************
 * ************************************ Write Functions *******************************************
 * ************************************************************************************************
 */
// Numeric implementation of our write data function
// Only accepts numerical values. Integers, floats, or doubles
template<typename T>
void WriteH5::writeData(std::vector<T> data)
{
   Exception::dontPrint();

   uint itr = 0; // Used to ensure we don't get stuck in an infinite loop
   uint npts = data.size(); // size of our data
   auto *a = new T[npts]; // convert to an array
   char* type = (char*)(typeid(a[0]).name());
   int vrank = 1; // since we are using std::vectors we are storing everything in one dimension

   // convert std::vector to array. H5 does not seem to like the pointer implementation
   for (size_t i = 0; i < npts; i++)
      a[i] = data[i];
   // conventional syntax for H5 data writing
   hsize_t dims[1];
   dims[0] = npts;
   // Let's make sure we are doing what we want and output it to the std output

   // We need to set these parameters for the H5 data file writing
   const H5std_string FILE_NAME(WriteH5::filename);
   H5std_string DATASET_NAME(WriteH5::variable);
   // loop here will check if the file exists. 
   while (true)
   {
      // This assumes that the file already exists and will then write to the file
      try
      {
         H5File file(FILE_NAME, H5F_ACC_RDWR);
         DataSpace dsp = DataSpace(vrank,dims);
         // int
         if ( type == (char*)typeid(int).name() )
         {
            DataSet dset = file.createDataSet(DATASET_NAME, PredType::STD_I32LE, dsp);
            dset.write(a, PredType::STD_I32BE);
            dset.close();
         }
         // uint
         else if ( type == (char*)typeid(uint).name() )
         {
            DataSet dset = file.createDataSet(DATASET_NAME, PredType::STD_U32LE, dsp);
            dset.write(a, PredType::STD_U32LE);
            dset.close();
         }
         // float
         else if ( type == (char*)typeid(float).name() )
         {
            DataSet dset = file.createDataSet(DATASET_NAME, PredType::IEEE_F32LE, dsp);
            dset.write(a, PredType::IEEE_F32LE);
            dset.close();
         }
         // double
         else if ( type == (char*)typeid(double).name() )
         {
            DataSet dset = file.createDataSet(DATASET_NAME, PredType::IEEE_F64LE, dsp);
            dset.write(a, PredType::IEEE_F64LE);
            dset.close();
         }
         else
         {
            std::cout << "Unknown data type! EXITING" << std::endl;
            exit(1);
         }

         // remember to close everything and delete our arrays
         dsp.close();
         file.close();
         delete[] a;
         break;
      }
      // Here we are catching if the file does not exist. We will then create a new file and return
      // back to the try statement
      catch (FileIException error)
      {
         H5File file(FILE_NAME, H5F_ACC_TRUNC);
         file.close();
         // Just some warning that we have gone through this catch
         itr++;
         // This is to prevent us from getting caught in an infinite loop. While (true) loops
         // are useful, but they can be dangerous. Always ensure some escape sequence. Could
         // just use a for loop
         if ( itr > 3) 
         {
            std::cout << "We've tried too many times in the Int writing sequence" << std::endl;
            break;
         }
      }
   }
}

void WriteH5::createGroup(std::string groupName)
{
   try
   {
      H5std_string FILE_NAME(WriteH5::filename);
      std::istringstream ss(groupName);
      std::string token;
      std::vector<std::string> groupSections;
      while ( std::getline(ss, token, '/') )
         groupSections.push_back(token);
      std::string mygroup;
      for ( size_t i = 0; i < groupSections.size(); i++ )
      {
         mygroup.append("/");
         mygroup.append(groupSections[i]);
         if ( mygroup != "/" )
         {
            H5File file(FILE_NAME, H5F_ACC_RDWR);
            Group group(file.createGroup(mygroup));
            group.close();
            file.close();
         }
      }
   }
   catch (FileIException error)
   {
      error.printError();
   }
   catch (GroupIException error)
   {
      error.printError();
   }
}

/*
 * ************************************************************************************************
 * ************************************ Read Functions ********************************************
 * ************************************************************************************************
 */

// Our int loading function
std::vector<int> LoadH5::getDataint() const
{
   try
   {
      Exception::dontPrint();
      //std::cout << "Getting int data" << std::endl;
      H5std_string FILE_NAME(LoadH5::filename);
      H5File file(FILE_NAME, H5F_ACC_RDONLY); // Only reads
      DataSet dataset = file.openDataSet(LoadH5::variable);
      DataType datatype = dataset.getDataType();
      DataSpace dataspace = dataset.getSpace();
      const int npts = dataspace.getSimpleExtentNpoints(); // Gets length of data
      H5T_class_t classt = datatype.getClass(); // Gets the data type of the data
      // Let's make a quick error check
      if ( classt != 0 )
      {
         std::cout << LoadH5::variable << " is not an int... you can't save this as an int." << std::endl;
         exit(1);
      }
      int *data = new int[npts]; // allocate at run time what the size will be
      IntType itype = dataset.getIntType();
      H5std_string order_string;
      H5T_order_t order = itype.getOrder( order_string );
      size_t size = itype.getSize();
      if ( (order_string == "Little endian byte ordering (0)" || order == 0) && size == 1 )
         dataset.read(data, PredType::STD_I8LE); // Our standard integer
      else if ( (order_string == "Little endian byte order_stringing (0)" || order== 0) && size == 2 )
         dataset.read(data, PredType::STD_I16LE); // Our standard integer
      else if ( (order_string == "Little endian byte order_stringing (0)" || order== 0) && size == 4 )
         dataset.read(data, PredType::STD_I32LE); // Our standard integer
      else if ( (order_string == "Little endian byte order_stringing (0)" || order== 0) && size == 8 ) 
         dataset.read(data, PredType::STD_I64LE);
      else if ( (order_string == "Big endian byte order_stringing (1)" || order== 1) && size == 1 )
         dataset.read(data, PredType::STD_I8BE); // Our standard integer
      else if ( (order_string == "Big endian byte order_stringing (1)" || order== 1) && size == 2 )
         dataset.read(data, PredType::STD_I16BE); // Our standard integer
      else if ( (order_string == "Big endian byte order_stringing (1)" || order== 1) && size == 4 )
         dataset.read(data, PredType::STD_I32BE);
      else if ( (order_string == "Big endian byte order_stringing (1)" || order== 1) && size == 8 )
         dataset.read(data, PredType::STD_I64BE);
      else 
         std::cout << "Did not find data type" << std::endl;
      std::vector<int> v(data, data + npts); // Arrays are nice, but vectors are better
      // Manage our memory properly
      delete[] data;
      dataspace.close();
      datatype.close();
      dataset.close();
      file.close();
      return v;
   }
   catch (FileIException error)
   {
      error.printError();
      std::vector<int> err;
      return err;
   }
   catch (GroupIException error)
   {
      error.printError();
      std::vector<int> err;
      return err;
   }
}

// Same as our int function, but with float. Uses IEEE_F32BE
std::vector<float> LoadH5::getDatafloat() const
{
   try
   {
      Exception::dontPrint();
      //std::cout << "Getting float data" << std::endl;
      H5std_string FILE_NAME(LoadH5::filename);
      H5File file(FILE_NAME, H5F_ACC_RDONLY);
      DataSet dataset = file.openDataSet(LoadH5::variable);
      DataType datatype = dataset.getDataType();
      DataSpace dataspace = dataset.getSpace();
      const int npts = dataspace.getSimpleExtentNpoints();
      H5T_class_t classt = datatype.getClass();
      if ( classt != 1 )
      {
         std::cout << LoadH5::variable << " is not a float... you can't save this as a float." << std::endl;
         exit(1);
      }
      FloatType ftype = dataset.getFloatType();
      H5std_string order_string;
      H5T_order_t order = ftype.getOrder( order_string);
      size_t size = ftype.getSize();
      float *data = new float[npts];
      if ( (order_string == "Little endian byte order_stringing (0)" || order == 0) && size == 4 )
         dataset.read(data, PredType::IEEE_F32LE); // Our standard integer
      else if (( order_string == "Little endian byte order_stringing (0)" || order == 0) && size == 8 ) 
      {
         dataset.read((float*)data, PredType::IEEE_F64LE);
         std::cout << "NOTE: This is actually double data. We are casting to float" << std:: endl;
      }
      else if ( (order_string == "Big endian byte order_stringing (1)" || order == 1) && size == 4 )
         dataset.read(data, PredType::IEEE_F32BE);
      else if ( (order_string == "Big endian byte order_stringing (1)" || order == 1) && size == 8 )
      {
         std::cout << "NOTE: This is actually double data. We are casting to float" << std:: endl;
         dataset.read((float*)data, PredType::IEEE_F64BE);
      }
      else 
         std::cout << "Did not find data type" << std::endl;
      std::vector<float> v(data, data + npts);
      delete[] data;
      dataspace.close();
      datatype.close();
      dataset.close();
      file.close();
      return v;
   }
   catch (FileIException error)
   {
      error.printError();
      std::vector<float> err;
      return err;
   }
   catch (GroupIException error)
   {
      error.printError();
      std::vector<float> err;
      return err;
   }
}

// Same as our int function, but with double
std::vector<double> LoadH5::getDatadouble() const
{
   try
   {
      Exception::dontPrint();
      H5std_string FILE_NAME(LoadH5::filename);
      H5File file(FILE_NAME, H5F_ACC_RDONLY);
      DataSet dataset = file.openDataSet(LoadH5::variable);
      DataType datatype = dataset.getDataType();
      DataSpace dataspace = dataset.getSpace();
      const int npts = dataspace.getSimpleExtentNpoints();
      H5T_class_t classt = datatype.getClass();
      if ( classt != 1 )
      {
         std::cout << LoadH5::variable << " is not a float... you can't save this as a float." << std::endl;
         exit(1);
      }
      FloatType ftype = dataset.getFloatType();
      H5std_string order_string;
      H5T_order_t order = ftype.getOrder( order_string);
      size_t size = ftype.getSize();
      double *data = new double[npts];
      if ( (char*)order == "Little endian byte ordering (0)" && size == 4 )
      {
         std::cout << "NOTE: This is actually float data. We are casting to double" << std:: endl;
         dataset.read((double*)data, PredType::IEEE_F32LE); // Our standard integer
      }
      else if ( (order_string == "Little endian byte ordering (0)" || order == 0)&& size == 8 ) 
         dataset.read(data, PredType::IEEE_F64LE);
      else if ( (order_string == "Big endian byte ordering (1)" || order == 1 )&& size == 4 )
      {
         std::cout << "NOTE: This is actually float data. We are casting to double" << std:: endl;
         dataset.read((double*)data, PredType::IEEE_F32BE);
      }
      else if ( (char*)order == "Big endian byte ordering (1)" && size == 8 )
         dataset.read((double*)data, PredType::IEEE_F64BE);
      else 
         std::cout << "Did not find data type" << std::endl;
      std::vector<double> v(data, data + npts);
      delete[] data;
      dataspace.close();
      datatype.close();
      dataset.close();
      file.close();
      return v;
   }
   catch (FileIException error)
   {
      error.printError();
      std::vector<double> err;
      return err;
   }
   catch (GroupIException error)
   {
      error.printError();
      std::vector<double> err;
      return err;
   }
}
