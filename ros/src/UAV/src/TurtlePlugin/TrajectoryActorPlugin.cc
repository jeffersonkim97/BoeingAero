/*
 * Copyright (C) 2018 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include <functional>

#include <ignition/math/Pose3.hh>
#include <ignition/math/Vector3.hh>

#include <gazebo/common/Animation.hh>
#include <gazebo/common/Console.hh>
#include <gazebo/common/KeyFrame.hh>
#include <gazebo/physics/physics.hh>

#include "TrajectoryActorPlugin.hh"

using namespace gazebo;
using namespace servicesim;
GZ_REGISTER_MODEL_PLUGIN(servicesim::TrajectoryActorPlugin)

class servicesim::TrajectoryActorPluginPrivate
{
  /// \brief Pointer to the actor.
  public: physics::ActorPtr actor{nullptr};

  /// \brief Velocity of the actor
  public: double velocity{0.8};

  /// \brief List of connections such as WorldUpdateBegin
  public: std::vector<event::ConnectionPtr> connections;

  /// \brief List of targets
  public: std::vector<ignition::math::Pose3d> targets;

  /// \brief Current target index
  public: unsigned int currentTarget{0};

  /// \brief Radius in meters around target pose where we consider it was
  /// reached.
  public: double targetRadius{0.5};

  /// \brief Margin by which to increase an obstacle's bounding box on every
  /// direction (2x per axis).
  public: double obstacleMargin{0.5};

  /// \brief Time scaling factor. Used to coordinate translational motion
  /// with the actor's walking animation.
  public: double animationFactor{5.1};

  /// \brief Time of the last update.
  public: common::Time lastUpdate;

  /// \brief Time when the corner starts
  public: common::Time firstCornerUpdate;

  /// \brief List of models to ignore when checking collisions.
  public: std::vector<std::string> ignoreModels;

  /// \brief Animation for corners
  public: common::PoseAnimation *cornerAnimation{nullptr};
};

/////////////////////////////////////////////////
TrajectoryActorPlugin::TrajectoryActorPlugin()
    : dataPtr(new TrajectoryActorPluginPrivate)
{
}

/////////////////////////////////////////////////
void TrajectoryActorPlugin::Load(physics::ModelPtr _model, sdf::ElementPtr _sdf)
{
  this->dataPtr->actor = boost::dynamic_pointer_cast<physics::Actor>(_model);

  this->dataPtr->connections.push_back(event::Events::ConnectWorldUpdateBegin(
      std::bind(&TrajectoryActorPlugin::OnUpdate, this, std::placeholders::_1)));

  // Read in the velocity
  if (_sdf->HasElement("velocity"))
    this->dataPtr->velocity = _sdf->Get<double>("velocity");

  // Read in the target poses
  auto targetElem = _sdf->GetElement("target");
  while (targetElem)
  {
    this->dataPtr->targets.push_back(targetElem->Get<ignition::math::Pose3d>());
    targetElem = targetElem->GetNextElement("target");
  }

  // Read in the target mradius
  if (_sdf->HasElement("target_radius"))
    this->dataPtr->targetRadius = _sdf->Get<double>("target_radius");

  // Read in the obstacle margin
  if (_sdf->HasElement("obstacle_margin"))
    this->dataPtr->obstacleMargin = _sdf->Get<double>("obstacle_margin");

  // Read in the animation factor
  if (_sdf->HasElement("animation_factor"))
    this->dataPtr->animationFactor = _sdf->Get<double>("animation_factor");

  // Add our own name to models we should ignore when avoiding obstacles.
  this->dataPtr->ignoreModels.push_back(this->dataPtr->actor->GetName());

  // Read in the other obstacles to ignore
  if (_sdf->HasElement("ignore_obstacle"))
  {
    auto ignoreElem = _sdf->GetElement("ignore_obstacle");
    while (ignoreElem)
    {
      auto name = ignoreElem->Get<std::string>();
      this->dataPtr->ignoreModels.push_back(name);
      ignoreElem = ignoreElem->GetNextElement("ignore_obstacle");
    }
  }

  // Read in the animation name
  std::string animation{"animation"};
  if (_sdf->HasElement("animation"))
    animation = _sdf->Get<std::string>("animation");

  auto skelAnims = this->dataPtr->actor->SkeletonAnimations();
  if (skelAnims.find(animation) == skelAnims.end())
  {
    gzerr << "Skeleton animation [" << animation << "] not found in Actor."
          << std::endl;
  }
  else
  {
    // Set custom trajectory
    gazebo::physics::TrajectoryInfoPtr trajectoryInfo(new physics::TrajectoryInfo());
    trajectoryInfo->type = animation;
    trajectoryInfo->duration = 1.0;

    this->dataPtr->actor->SetCustomTrajectory(trajectoryInfo);
  }
}

/////////////////////////////////////////////////
void TrajectoryActorPlugin::Reset()
{
  this->dataPtr->currentTarget = 0;
  this->dataPtr->cornerAnimation = nullptr;
  this->dataPtr->lastUpdate = common::Time::Zero;
}

/////////////////////////////////////////////////
bool TrajectoryActorPlugin::ObstacleOnTheWay() const
{
  auto actorPose = this->dataPtr->actor->WorldPose().Pos();
  auto world = this->dataPtr->actor->GetWorld();

  // Iterate over all models in the world
  for (unsigned int i = 0; i < world->ModelCount(); ++i)
  {
    // Skip models we're ignoring
    auto model = world->ModelByIndex(i);
    if (std::find(this->dataPtr->ignoreModels.begin(),
                  this->dataPtr->ignoreModels.end(), model->GetName()) !=
                  this->dataPtr->ignoreModels.end())
    {
      continue;
    }

    // Obstacle's bounding box
    auto bb = model->BoundingBox();

    // Increase box by margin
    bb.Min() -= ignition::math::Vector3d::One * this->dataPtr->obstacleMargin;
    bb.Max() += ignition::math::Vector3d::One * this->dataPtr->obstacleMargin;

    // Increase vertically
    bb.Min().Z() -= 5;
    bb.Max().Z() += 5;

    // Check
    if (bb.Contains(actorPose))
    {
      return true;
    }

    // TODO: Improve obstacle avoidance. Some ideas: check contacts, ray-query
    // the path forward, check against bounding box of each collision shape...
  }
  return false;
}

/////////////////////////////////////////////////
void TrajectoryActorPlugin::UpdateTarget()
{
  // Current actor position
  auto actorPos = this->dataPtr->actor->WorldPose().Pos();

  // Current target
  auto target = this->dataPtr->targets[this->dataPtr->currentTarget].Pos();

  // 2D distance to target
  auto posDiff = target - actorPos;
  posDiff.Z(0);

  double distance = posDiff.Length();

  // Still far from target?
  if (distance > this->dataPtr->targetRadius)
    return;

  // Move on to next target
  this->dataPtr->currentTarget++;
  if (this->dataPtr->currentTarget > this->dataPtr->targets.size() - 1)
    this->dataPtr->currentTarget = 0;
}

/////////////////////////////////////////////////
void TrajectoryActorPlugin::OnUpdate(const common::UpdateInfo &_info)
{
  // Time delta
  double dt = (_info.simTime - this->dataPtr->lastUpdate).Double();

  this->dataPtr->lastUpdate = _info.simTime;

  // Don't move if there's an obstacle on the way
  if (this->ObstacleOnTheWay())
    return;

  // Target
  this->UpdateTarget();

  // Current pose - actor is oriented Y-up and Z-front
  auto actorPose = this->dataPtr->actor->WorldPose();

  // Current target
  auto targetPose = this->dataPtr->targets[this->dataPtr->currentTarget];

  // Direction to target
  auto dir = (targetPose.Pos() - actorPose.Pos()).Normalize();

  // TODO: generalize for actors facing other directions
  auto currentYaw = actorPose.Rot().Yaw();

  // Difference to target
  ignition::math::Angle yawDiff = atan2(dir.Y(), dir.X()) + IGN_PI_2 - currentYaw;
  yawDiff.Normalize();

  // Rotate if needed
  if (std::abs(yawDiff.Radian()) > IGN_DTOR(10))
  {
    // Not rotating yet
    if (!this->dataPtr->cornerAnimation)
    {
      // Previous target (we assume we just reached it)
      int previousTarget = this->dataPtr->currentTarget - 1;
      if (previousTarget < 0)
        previousTarget = this->dataPtr->targets.size() - 1;

      auto prevTargetPos = this->dataPtr->targets[previousTarget].Pos();

      // Direction from previous target to current target
      auto prevDir = (targetPose.Pos() - prevTargetPos).Normalize() *
          this->dataPtr->targetRadius;

      // Curve end point
      auto endPt = prevTargetPos + prevDir;

      // Total time to finish the curve, we try to keep about the same speed,
      // it will be a bit slower because we're doing an arch, not a straight
      // line
      auto curveDist = (endPt - actorPose.Pos()).Length();
      auto curveTime = curveDist / this->dataPtr->velocity;

      // Use pose animation for spline
      this->dataPtr->cornerAnimation = new common::PoseAnimation("anim", curveTime, false);

      // Start from actor's current pose
      auto start = this->dataPtr->cornerAnimation->CreateKeyFrame(0.0);
      start->Translation(actorPose.Pos());
      start->Rotation(actorPose.Rot());

      // End of curve
      auto endYaw = atan2(prevDir.Y(), prevDir.X()) + IGN_PI_2;
      auto end = this->dataPtr->cornerAnimation->CreateKeyFrame(curveTime);
      end->Translation(endPt);
      end->Rotation(ignition::math::Quaterniond(IGN_PI_2, 0, endYaw));

      this->dataPtr->firstCornerUpdate = _info.simTime;
    }

    // Get point in curve
    auto cornerDt = (_info.simTime - this->dataPtr->firstCornerUpdate).Double();
    common::PoseKeyFrame pose(cornerDt);
    this->dataPtr->cornerAnimation->SetTime(cornerDt);
    this->dataPtr->cornerAnimation->GetInterpolatedKeyFrame(pose);

    actorPose.Pos() = pose.Translation();
    actorPose.Rot() = ignition::math::Quaterniond(IGN_PI_2, 0, pose.Rotation().Yaw());
  }
  else
  {
    this->dataPtr->cornerAnimation = nullptr;

    actorPose.Pos() += dir * this->dataPtr->velocity * dt;

    // TODO: remove hardcoded roll
    actorPose.Rot() = ignition::math::Quaterniond(IGN_PI_2, 0, currentYaw + yawDiff.Radian());
  }

  // Distance traveled is used to coordinate motion with the walking
  // animation
  double distanceTraveled = (actorPose.Pos() -
      this->dataPtr->actor->WorldPose().Pos()).Length();

  // Update actor
  this->dataPtr->actor->SetWorldPose(actorPose, false, false);
  this->dataPtr->actor->SetScriptTime(this->dataPtr->actor->ScriptTime() +
    (distanceTraveled * this->dataPtr->animationFactor));
}