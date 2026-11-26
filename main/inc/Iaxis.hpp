#pragma once 

class Iaxis
{
public:
    virtual ~Iaxis() = default;

    virtual void move_to_angle(float angle) = 0;

    virtual void move_by_delta(float delta) = 0;

    virtual bool is_stopped() = 0;

    virtual void set_current_pos_as_azimuth(float azimuth_deg) {};


};