#include "rosettelab/curves/harmonograph.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace rosettelab::curves {
namespace {

double radians(const double degrees) { return degrees * std::numbers::pi / 180.0; }

core::Point rotate(const core::Point p, const double angle)
{
    const double c = std::cos(angle), s = std::sin(angle);
    return {p.x*c-p.y*s, p.x*s+p.y*c};
}

core::Point point_at(const HarmonographParameters& p, const double t)
{
    return rotate({
        p.amplitude_x * std::sin(p.frequency_x*t+radians(p.phase_x_degrees)) * std::exp(-p.damping_x*t),
        p.amplitude_y * std::sin(p.frequency_y*t+radians(p.phase_y_degrees)) * std::exp(-p.damping_y*t),
    }, radians(p.rotation_degrees));
}

core::Point derivative_at(const HarmonographParameters& p, const double t)
{
    const double px = p.frequency_x*t+radians(p.phase_x_degrees);
    const double py = p.frequency_y*t+radians(p.phase_y_degrees);
    return rotate({
        p.amplitude_x*std::exp(-p.damping_x*t)*(p.frequency_x*std::cos(px)-p.damping_x*std::sin(px)),
        p.amplitude_y*std::exp(-p.damping_y*t)*(p.frequency_y*std::cos(py)-p.damping_y*std::sin(py)),
    }, radians(p.rotation_degrees));
}

core::Point cubic_at(const core::CubicBezier& c, const double u)
{
    const double v=1.0-u;
    return {v*v*v*c.start.x+3*v*v*u*c.control1.x+3*v*u*u*c.control2.x+u*u*u*c.end.x,
            v*v*v*c.start.y+3*v*v*u*c.control1.y+3*v*u*u*c.control2.y+u*u*u*c.end.y};
}

core::CubicBezier segment_at(const HarmonographParameters& p, const double a, const double b)
{
    const auto p0=point_at(p,a), p3=point_at(p,b), d0=derivative_at(p,a), d1=derivative_at(p,b);
    const double scale=(b-a)/3.0;
    return {p0,{p0.x+scale*d0.x,p0.y+scale*d0.y},{p3.x-scale*d1.x,p3.y-scale*d1.y},p3};
}

double error(const HarmonographParameters& p, const core::CubicBezier& c, const double a, const double b)
{
    double result=0.0;
    for (const double u : {0.25,0.5,0.75}) {
        const auto exact=point_at(p,a+(b-a)*u), approximate=cubic_at(c,u);
        result=std::max(result,std::hypot(exact.x-approximate.x,exact.y-approximate.y));
    }
    return result;
}

void validate(const HarmonographParameters& p, const double tolerance)
{
    if (!std::isfinite(p.amplitude_x)||p.amplitude_x<=0.0||!std::isfinite(p.amplitude_y)||p.amplitude_y<=0.0||
        !std::isfinite(p.frequency_x)||p.frequency_x<=0.0||!std::isfinite(p.frequency_y)||p.frequency_y<=0.0||
        !std::isfinite(p.phase_x_degrees)||!std::isfinite(p.phase_y_degrees)||
        !std::isfinite(p.damping_x)||p.damping_x<0.0||!std::isfinite(p.damping_y)||p.damping_y<0.0||
        !std::isfinite(p.duration)||p.duration<=0.0||!std::isfinite(p.rotation_degrees)||
        !std::isfinite(tolerance)||tolerance<=0.0)
        throw std::invalid_argument("Harmonograph parameters must be finite and positive where required");
}

} // namespace

core::BezierPath generate_harmonograph_bezier(const HarmonographParameters& p, const double tolerance)
{
    validate(p,tolerance);
    core::BezierPath result;
    result.closed=false;
    const auto intervals=static_cast<std::size_t>(std::max(8.0,std::ceil(p.duration*std::max(p.frequency_x,p.frequency_y)/std::numbers::pi)));
    constexpr std::size_t maximum_segments=100'000;
    constexpr int maximum_depth=18;
    const auto append=[&](auto&& self,const double a,const double b,const int depth)->void {
        const auto segment=segment_at(p,a,b);
        if (depth>=maximum_depth||error(p,segment,a,b)<=tolerance) {
            if (result.segments.size()>=maximum_segments) throw std::length_error("Harmonograph exceeds the supported Bezier segment count");
            result.segments.push_back(segment); return;
        }
        const double middle=(a+b)/2.0;
        self(self,a,middle,depth+1); self(self,middle,b,depth+1);
    };
    for (std::size_t i=0;i<intervals;++i) append(append,p.duration*i/intervals,p.duration*(i+1)/intervals,0);
    return result;
}

} // namespace rosettelab::curves
