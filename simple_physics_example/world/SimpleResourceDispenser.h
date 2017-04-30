
#ifndef SIMPLERESOURCEDISPENSER_H
#define SIMPLERESOURCEDISPENSER_H

#include "geometry/Circle2D.h"
#include "geometry/Angle2D.h"
#include "geometry/Point2D.h"

#include "tools/math.h"
#include "tools/Random.h"

#include "SimpleResource.h"

#include <utility>



class SimpleResourceDispenser : public emp::PhysicsBodyOwner_Base<emp::PhysicsBody2D<emp::Circle>> {
private:
  using Body_t = emp::PhysicsBody2D<emp::Circle>;
  using Resource_t = SimpleResource;
  using Dispenser_t = SimpleResourceDispenser;
  using emp::PhysicsBodyOwner_Base<Body_t>::body;
  using emp::PhysicsBodyOwner_Base<Body_t>::has_body;

  int update_timer;

  int dispense_amount;
  double dispense_rate;
  std::pair<emp::Angle, emp::Angle> dispense_range;

  emp::BitVector affinity;
  double affinity_noise;
  double resource_value;
  double resource_radius;

  // Signal for dispensing.
  emp::Signal<void(Dispenser_t*)> dispenser_timer_signal;

public:
  SimpleResourceDispenser(const emp::Circle &_p,
                          int _dispense_amount = 0, double _dispense_rate = 1,
                          double _dispense_start_angle = 0.0,
                          double _dispense_end_angle = 2*emp::PI,
                          double _resource_value = 0.0,
                          double _resource_radius = 1.0,
                          const emp::BitVector & _affinity = emp::BitVector(1, false),
                          double _affinity_noise = 0.0)
  : update_timer(0), dispense_amount(_dispense_amount), dispense_rate(_dispense_rate),
    dispense_range(emp::Angle(_dispense_start_angle), emp::Angle(_dispense_end_angle)),
    affinity(_affinity), affinity_noise(_affinity_noise), resource_value(_resource_value),
    resource_radius(_resource_radius)
  {
    body = nullptr;
    has_body = false;
    AttachBody(new Body_t(_p));
    body->SetMass(10000);
    body->SetImmobile(true);
  }

  ~SimpleResourceDispenser() {

  }

  int GetDispenseAmount() const { return dispense_amount; }
  double GetDispenseRate() const { return dispense_rate; }
  const std::pair<emp::Angle, emp::Angle> & GetDispenseRange() const { return dispense_range; }
  const emp::Angle & GetDispenseStartAngle() const { return dispense_range.first; }
  const emp::Angle & GetDispenseEndAngle() const { return dispense_range.second; }
  const emp::BitVector & GetAffinity() const { return affinity; }
  double GetAffinityNoise() const { return affinity_noise; }
  double GetResourcevalue() const { return resource_value; }
  double GetResourceRadius() const { return resource_radius; }

  void SetDispenseAmount(int val) { dispense_amount = val; }
  void SetDispenseRate(double rate) { dispense_rate = rate; }
  void SetDispenseStartAngleRad(double ang) { dispense_range.first.SetRadians(ang); }
  void SetDispenseStartAngleDeg(double ang) { dispense_range.first.SetDegrees(ang); }
  void SetDispenseEndAngleRad(double ang) { dispense_range.second.SetRadians(ang); }
  void SetDispenseEndAngleDeg(double ang) { dispense_range.second.SetDegrees(ang); }
  void SetAffinity(const emp::BitVector & aff) { affinity = aff; }
  void SetAffinityNoise(double noise) { affinity_noise = noise; }
  void SetResourceValue(double value) { resource_value = value; }
  void SetResourceRadius(double radius) { resource_radius = radius; }

  void RegisterDispenserTimerCallback(std::function<void(Dispenser_t*)> fun) {
    dispenser_timer_signal.AddAction(fun);
  }

  // This function is serious business. (Dispenser is not responsible for resource memory cleanup)
  emp::vector<Resource_t*> Dispense(emp::Random *random_ptr) {
    // Will dispense (return) a number of resources equal to resource amount.
    // Will dispense resources randomly around dispenser (from start to end angle).
    // Dispensed resources will have affinities equal to dispenser affinity w/noise. etc etc.
    if (dispense_amount < 1) return emp::vector<Resource_t*>(0);
    emp::vector<Resource_t*> dispense(dispense_amount);
    for (int i = 0; i < dispense_amount; ++i) {
      const double sang = emp::Min(dispense_range.first.AsRadians(), dispense_range.second.AsRadians());
      const double eang = emp::Max(dispense_range.first.AsRadians(), dispense_range.second.AsRadians());
      emp::Angle dispense_angle(random_ptr->GetDouble(sang, eang));
      auto offset = dispense_angle.GetPoint(body->GetShape().GetRadius() + resource_radius);
      Resource_t *res = new Resource_t(emp::Circle(body->GetShape().GetCenter(), resource_radius));
      res->SetValue(resource_value);
      res->SetAffinity(affinity);
      res->GetBody().GetShape().Translate(offset);
      res->GetBody().SetVelocity(emp::Point(offset, random_ptr->GetDouble(0.0, 1.0))); // Random outward velocity.
      dispense[i] = res;
    }
    return dispense;
  }

  void Evaluate() override {
    emp::PhysicsBodyOwner_Base<Body_t>::Evaluate();
    ++update_timer;
    if (update_timer >= dispense_rate) {
      // Trigger dispense.
      dispenser_timer_signal.Trigger(this);
      // Reset timer.
      update_timer = 0;
    }
  }
};

#endif
