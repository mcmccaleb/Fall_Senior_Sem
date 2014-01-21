/*
** Copyright (C) 1998-2010 George Tzanetakis <gtzan@cs.uvic.ca>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "Cascade.h"


using std::ostringstream;
using std::vector;

using namespace Marsyas;

Cascade::Cascade(mrs_string name):MarSystem("Cascade", name)
{
  isComposite_ = true;
}

Cascade::~Cascade()
{
  deleteSlices();
}

void
Cascade::deleteSlices()
{
  vector<realvec *>::const_iterator iter;
  for (iter = slices_.begin(); iter != slices_.end(); iter++) {
    delete *(iter);
  }
  slices_.clear();
}

MarSystem*
Cascade::clone() const
{
  return new Cascade(*this);
}

void
Cascade::myUpdate(MarControlPtr sender)
{
  child_count_t child_count = marsystems_.size();
  if (child_count)
  {
    //propagate in flow controls to first child
    marsystems_[0]->setctrl("mrs_natural/inObservations", inObservations_);
    marsystems_[0]->setctrl("mrs_natural/inSamples", inSamples_);
    marsystems_[0]->setctrl("mrs_real/israte", israte_);
    marsystems_[0]->setctrl("mrs_string/inObsNames", inObsNames_);
    marsystems_[0]->update();

    // update dataflow component MarSystems in order
    ostringstream oss;
    oss << marsystems_[0]->getctrl("mrs_string/onObsNames");
    mrs_natural onObservations = marsystems_[0]->getctrl("mrs_natural/onObservations")->to<mrs_natural>();
    for (child_count_t i=1; i < child_count; ++i)
    {
      marsystems_[i]->setctrl("mrs_natural/inSamples", marsystems_[i-1]->getctrl("mrs_natural/onSamples"));
      marsystems_[i]->setctrl("mrs_natural/inObservations", marsystems_[i-1]->getctrl("mrs_natural/onObservations"));
      marsystems_[i]->setctrl("mrs_real/israte", marsystems_[i-1]->getctrl("mrs_real/osrate"));
      marsystems_[i]->update();
      oss << marsystems_[i]->getctrl("mrs_string/onObsNames");
      onObservations += marsystems_[i]->getctrl("mrs_natural/onObservations")->to<mrs_natural>();
    }

    //forward flow propagation
    setctrl(ctrl_onSamples_, marsystems_[0]->getctrl("mrs_natural/onSamples"));
    setctrl(ctrl_onObservations_, onObservations);
    setctrl(ctrl_osrate_, marsystems_[0]->getctrl("mrs_real/osrate"));
    setctrl(ctrl_onObsNames_, oss.str());

    // update buffers between components
    if (slices_.size() < child_count)
    {
      slices_.resize(child_count, NULL);
    }

    for (child_count_t i = 0; i < child_count; ++i)
    {
      if (slices_[i] != NULL)
      {
        if ((slices_[i])->getRows() != marsystems_[i]->getctrl("mrs_natural/onObservations")->to<mrs_natural>() ||
            (slices_[i])->getCols() != marsystems_[i]->getctrl("mrs_natural/onSamples")->to<mrs_natural>())
        {
          delete slices_[i];
          slices_[i] = new realvec(marsystems_[i]->getctrl("mrs_natural/onObservations")->to<mrs_natural>(),
                                   marsystems_[i]->getctrl("mrs_natural/onSamples")->to<mrs_natural>());
        }
      }
      else
      {
        slices_[i] = new realvec(marsystems_[i]->getctrl("mrs_natural/onObservations")->to<mrs_natural>(),
                                 marsystems_[i]->getctrl("mrs_natural/onSamples")->to<mrs_natural>());
      }

      (slices_[i])->setval(0.0);
    }
  }
  else //if composite is empty...
    MarSystem::myUpdate(sender);
}

void
Cascade::myProcess(realvec& in, realvec& out)
{
  mrs_natural o,t;
  mrs_natural outIndex = 0;
  mrs_natural localIndex = 0;
  child_count_t child_count = marsystems_.size();

  if (child_count == 1)
  {
    marsystems_[0]->process(in, out);
  }
  else if (child_count > 1)
  {
    marsystems_[0]->process(in, *(slices_[0]));
    localIndex = marsystems_[0]->getctrl("mrs_natural/onObservations")->to<mrs_natural>();
    for (o = 0; o < localIndex; o++)
    {
      for (t = 0; t < onSamples_; t++)
      {
        out(outIndex + o,t) = (*(slices_[0]))(o,t);
      }
    }
    outIndex += localIndex;
    for (child_count_t i = 1; i < child_count; ++i)
    {
      marsystems_[i]->process(*(slices_[i-1]), *(slices_[i]));
      localIndex = marsystems_[i]->getctrl("mrs_natural/onObservations")->to<mrs_natural>();
      for (o = 0; o < localIndex; o++)
      {
        for (t = 0; t < onSamples_; t++)
        {
          out(outIndex + o,t) = (*(slices_[i]))(o,t);
        }
      }
      outIndex += localIndex;
    }
  }
  else if(child_count == 0) //composite has no children!
  {
    MRSWARN("Cascade::process: composite has no children MarSystems - passing input to output without changes.");
    out = in;
  }
}
