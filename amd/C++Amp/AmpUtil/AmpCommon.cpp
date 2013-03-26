/**********************************************************************
Copyright ©2012 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

•	Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
•	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/

/******************************************************************************
* Included header files                                                       *
******************************************************************************/
#include <AmpCommon.h>

/******************************************************************************
* namespace ampsdk                                                            *
******************************************************************************/
namespace ampsdk
{
    /**************************************************************************
    * Implementation of AmpCommon::~AmpCommon()                               *
    **************************************************************************/
    AmpCommon::~AmpCommon()
    {
        while(!_timers.empty())
        {
            Timer *temp = _timers.back();
            _timers.pop_back();
            delete temp;
        }
    }

    /******************************************************************************
    * Implementation of AmpCommon::printArray()                                   *
    ******************************************************************************/
    template<typename T> 
    void AmpCommon::printArray(
        std::string header, 
        const T * data, 
        const int width,
        const int height) const
    {
        std::cout<<"\n"<<header<<"\n";
        for(int i = 0; i < height; i++)
        {
            for(int j = 0; j < width; j++)
            {
                std::cout<<data[i*width+j]<<" ";
            }
            std::cout<<"\n";
        }
        std::cout<<"\n";
    }

    /**************************************************************************
    * Implementation of AmpCommon::printArray()                               *
    **************************************************************************/
    template<typename T> 
    void AmpCommon::printArray(
        std::string header, 
        const std::vector<T>& data, 
        const int width,
        const int height) const
    {
        std::cout<<"\n"<<header<<"\n";
        for(int i = 0; i < height; i++)
        {
            for(int j = 0; j < width; j++)
            {
                std::cout<<data[i*width+j]<<" ";
            }
            std::cout<<"\n";
        }
        std::cout<<"\n";
    }

    /**************************************************************************
    * Implementation of AmpCommon::compare()                                  *
    **************************************************************************/
    bool AmpCommon::compare(const float *refData, const float *data, 
                        const int length, const float epsilon)
    {
        float error = 0.0f;
        float ref = 0.0f;

        for(int i = 1; i < length; ++i) 
        {
            float diff = refData[i] - data[i];
            error += diff * diff;
            ref += refData[i] * refData[i];
        }

        float normRef =::sqrtf((float) ref);
        if (::fabs((float) ref) < 1e-7f) {
            return false;
        }
        float normError = ::sqrtf((float) error);
        error = normError / normRef;

        return error < epsilon;
    }

    /**************************************************************************
    * Implementation of AmpCommon::compare()                                  *
    **************************************************************************/
    bool AmpCommon::compare(const double *refData, const double *data, 
                            const int length, const double epsilon)
    {
        double error = 0.0;
        double ref = 0.0;

        for(int i = 1; i < length; ++i) 
        {
            double diff = refData[i] - data[i];
            error += diff * diff;
            ref += refData[i] * refData[i];
        }

        double normRef =::sqrt((double) ref);
        if (::fabs((double) ref) < 1e-7) 
        {
            return false;
        }
        double normError = ::sqrt((double) error);
        error = normError / normRef;

        return error < epsilon;
    }

    /**************************************************************************
    * Implementation of AmpCommon::createTimer()                              *
    **************************************************************************/
    int AmpCommon::createTimer()
    {
        Timer* newTimer = new Timer;
        newTimer->_start = 0;
        newTimer->_clocks = 0;

    #ifdef _WIN32
        QueryPerformanceFrequency((LARGE_INTEGER*)&newTimer->_freq);
    #else
        newTimer->_freq = (long long)1.0E3;
    #endif
    
        _timers.push_back(newTimer);
        return (int)(_timers.size() - 1);
    }

    /**************************************************************************
    * Implementation of AmpCommon::resetTimer()                               *
    **************************************************************************/
    int AmpCommon::resetTimer(int handle)
    {
        if(handle >= (int)_timers.size())
        {
            error("Cannot reset timer. Invalid handle.");
            return -1;
        }

        (_timers[handle]->_start) = 0;
        (_timers[handle]->_clocks) = 0;
        return AMP_SUCCESS;
    }

    /**************************************************************************
    * Implementation of AmpCommon::startTimer()                               *
    **************************************************************************/
    int AmpCommon::startTimer(int handle)
    {
        if(handle >= (int)_timers.size())
        {
            error("Cannot reset timer. Invalid handle.");
            return AMP_FAILURE;
        }

    #ifdef _WIN32
        QueryPerformanceCounter((LARGE_INTEGER*)&(_timers[handle]->_start));	
    #else
        struct timeval s;
        gettimeofday(&s, 0);
        _timers[handle]->_start = (long long)s.tv_sec * 
                    (long long)1.0E3 + (long long)s.tv_usec / (long long)1.0E3;
    #endif

        return AMP_SUCCESS;
    }

    /**************************************************************************
    * Implementation of AmpCommon::stopTimer()                                *
    **************************************************************************/
    int AmpCommon::stopTimer(int handle)
    {
        long long n=0;

        if(handle >= (int)_timers.size())
        {
            error("Cannot reset timer. Invalid handle.");
            return AMP_FAILURE;
        }

    #ifdef _WIN32
        QueryPerformanceCounter((LARGE_INTEGER*)&(n));	
    #else
        struct timeval s;
        gettimeofday(&s, 0);
        n = (long long)s.tv_sec * (long long)1.0E3 +
                                       (long long)s.tv_usec / (long long)1.0E3;
    #endif

        n -= _timers[handle]->_start;
        _timers[handle]->_start = 0;
        _timers[handle]->_clocks += n;

        return AMP_SUCCESS;
    }

    /**************************************************************************
    * Implementation of AmpCommon::readTimer()                                *
    **************************************************************************/
    double AmpCommon::readTimer(int handle)
    {
        if(handle >= (int)_timers.size())
        {
            error("Cannot read timer. Invalid handle.");
            return AMP_FAILURE;
        }

        double reading = double(_timers[handle]->_clocks);
        reading = double(reading / _timers[handle]->_freq);

        return reading;
    }

    /**************************************************************************
    * Implementation of AmpCommon::printTable()                               *
    **************************************************************************/
    void AmpCommon::printTable(Table *t)
    {
        if(t == NULL)
        {
            error("Cannot print table, NULL pointer.");
            return;
        }

        int count = 0;
        /**********************************************************************
        * Skip delimiters at beginning and find first "non-delimiter"         *
        **********************************************************************/
        std::string::size_type curIndex = 
                                t->_dataItems.find_first_not_of(t->_delim, 0);
        std::string::size_type nextIndex = 
            t->_dataItems.find_first_of(t->_delim, curIndex);

        while (std::string::npos != nextIndex || std::string::npos != curIndex)
        {
            /******************************************************************
            * Found a token, add it to the vector. Skip delimiters. Find      *
            * next "non-delimiter"                                            *
            ******************************************************************/
            std::cout<<std::setw(t->_columnWidth)<<std::left
                     <<t->_dataItems.substr(curIndex, nextIndex - curIndex);				 
            curIndex = t->_dataItems.find_first_not_of(t->_delim, nextIndex);
            nextIndex = t->_dataItems.find_first_of(t->_delim, curIndex);

            count++;

            if(count%t->_numColumns==0)
                std::cout<<"\n";
        }	
    }

    /**************************************************************************
    * Implementation of AmpCommon::error()                                    *
    **************************************************************************/
    void  AmpCommon::error(const char* errorMsg)
    {    
        std::cout<<"Error: "<<errorMsg<<std::endl;
    }

    /**************************************************************************
    * Implementation of AmpCommon::error()                                    *
    **************************************************************************/
    void AmpCommon::error(std::string errorMsg)
    {
        std::cout<<"Error: "<<errorMsg<<std::endl;
    }

    /**************************************************************************
    * Template Instantiations                                                 *
    **************************************************************************/
    template 
    void AmpCommon::printArray<short>(const std::string, 
            const short*, int, int)const;
    template 
    void AmpCommon::printArray<short>(const std::string, 
                                 const std::vector<short>&, int, int)const;
    template 
    void AmpCommon::printArray<unsigned char>(const std::string, 
                                 const unsigned char*, int, int)const;
    template 
    void AmpCommon::printArray<unsigned char>(const std::string, 
                                  const std::vector<unsigned char>&, int, int)const; 
    template 
    void AmpCommon::printArray<unsigned int>(const std::string, 
            const unsigned int*, int, int)const;
    template 
    void AmpCommon::printArray<unsigned int>(const std::string, 
                                   const std::vector<unsigned int>&, int, int)const;
    template 
    void AmpCommon::printArray<int>(const std::string, 
        const int*, int, int)const;
    template 
    void AmpCommon::printArray<int>(const std::string, 
                                   const std::vector<int>&, int, int)const;
    template 
    void AmpCommon::printArray<long>(const std::string, 
                                   const long*, int, int)const;
    template 
    void AmpCommon::printArray<long>(const std::string, 
                                   const std::vector<long>&, int, int)const;
    template 
    void AmpCommon::printArray<float>(const std::string, 
                                   const float*, int, int)const;
    template 
    void AmpCommon::printArray<float>(const std::string, 
                                   const std::vector<float>&, int, int)const;
    template 
    void AmpCommon::printArray<double>(const std::string, 
                                   const double*, int, int)const;
    template 
    void AmpCommon::printArray<double>(const std::string, 
                                   const std::vector<double>&, int, int)const;

}