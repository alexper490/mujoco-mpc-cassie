// Copyright 2023 DeepMind Technologies Limited
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mjpc/estimators/buffer.h"

#include <vector>
#include <cstring>
#include <stdio.h> 

#include "mjpc/estimators/trajectory.h"
#include <mujoco/mujoco.h>

namespace mjpc {

// initialize
void Buffer::Initialize(mjModel* model, int max_length) {
  // sensor 
  sensor_.Initialize(model->nsensordata, 0);

  // sensor mask
  sensor_mask_.Initialize(model->nsensor, 0);

  // mask (for single time step)
  mask_.resize(model->nsensor);
  std::fill(mask_.begin(), mask_.end(), 1);

  // ctrl 
  ctrl_.Initialize(model->nu, 0);

  // time 
  time_.Initialize(1, 0);

  // maximum buffer length 
  max_length_ = max_length;
}

// reset
void Buffer::Reset() {
  // sensor 
  sensor_.Reset();
  sensor_.length_ = 0;

  // sensor mask
  sensor_mask_.Reset();
  sensor_mask_.length_ = 0;

  // mask
  std::fill(mask_.begin(), mask_.end(), 1); // set to true

  // ctrl 
  ctrl_.Reset();
  ctrl_.length_ = 0;

  // time 
  time_.Reset();
  time_.length_ = 0;
}

// update
void Buffer::Update(mjModel* model, mjData* data) {
  if (time_.length_ <= max_length_) { // fill buffer
    // time
    time_.data_[time_.length_++] = data->time;

    // ctrl
    mju_copy(ctrl_.data_.data() + ctrl_.length_ * model->nu,
              data->ctrl, model->nu);
    ctrl_.length_++;

    // sensor
    mju_copy(sensor_.data_.data() +
                  sensor_.length_ * model->nsensordata,
              data->sensordata, model->nsensordata);
    sensor_.length_++;

    // sensor mask 
    // TODO(taylor): external method must set mask
    std::memcpy(
        sensor_mask_.data_.data() + sensor_mask_.length_ * model->nsensor,
        mask_.data(), model->nsensor * sizeof(int));
    sensor_mask_.length_++;
  } else {  // update buffer
    // time
    time_.ShiftHeadIndex(1);
    time_.Set(&data->time, time_.length_ - 1);

    // ctrl
    ctrl_.ShiftHeadIndex(1);
    ctrl_.Set(data->ctrl, ctrl_.length_ - 1);

    // sensor
    sensor_.ShiftHeadIndex(1);
    sensor_.Set(data->sensordata, sensor_.length_ - 1);

    // sensor mask 
    // TODO(taylor): external method must set mask
    sensor_mask_.ShiftHeadIndex(1);
    sensor_mask_.Set(mask_.data(), sensor_.length_ - 1);
  }
}

 // update mask 
void Buffer::UpdateMask() {
  // TODO(taylor)
}

// print 
void Buffer::Print() {
  for (int i = 0; i < time_.length_; i++) {
    printf("(%i)\n", i);
    printf("\n");
    printf("time = %.4f\n", *time_.Get(i));
    printf("\n");
    printf("sensor = ");
    mju_printMat(sensor_.Get(i), 1, sensor_.dim_);
    printf("sensor mask = ");
    for (int j = 0; j < sensor_mask_.dim_; j++)
      printf("%i ", sensor_mask_.Get(i)[j]);
    printf("\n");
    printf("ctrl = ");
    mju_printMat(ctrl_.Get(i), 1, ctrl_.dim_);
    printf("\n");
  }
}

// length
int Buffer::Length() const { return time_.length_; }

}  // namespace mjpc