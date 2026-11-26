#pragma once
#include "shared_data.hpp"
 
class band_communication
{
    SharedNavigationData & shared_data_;

    public:

    band_communication(SharedNavigationData& shared_data) : shared_data_(shared_data)
    {
        
    }

    virtual ~band_communication() = default;

    virtual void  receive_data_to_shared_data() = 0;


};