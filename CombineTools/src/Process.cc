#include "CombineTools/interface/Process.h"
#include <iostream>
#include <string>
#include "boost/format.hpp"
#include "CombineTools/interface/Logging.h"

namespace ch {

Process::Process()
    : Object(),
      rate_(0.0),
      signal_(false),
      shape_(),
      pdf_(nullptr),
      data_(nullptr),
      norm_(nullptr) {
  }

Process::~Process() { }

void swap(Process& first, Process& second) {
  using std::swap;
  swap(static_cast<Object&>(first), static_cast<Object&>(second));
  swap(first.rate_, second.rate_);
  swap(first.signal_, second.signal_);
  swap(first.shape_, second.shape_);
  swap(first.pdf_, second.pdf_);
  swap(first.data_, second.data_);
  swap(first.norm_, second.norm_);
}

Process::Process(Process const& other)
    : Object(other),
      rate_(other.rate_),
      signal_(other.signal_),
      pdf_(other.pdf_),
      data_(other.data_),
      norm_(other.norm_) {
  TH1 *h = nullptr;
  if (other.shape_) {
    h = static_cast<TH1*>(other.shape_->Clone());
    h->SetDirectory(0);
  }
  shape_ = std::unique_ptr<TH1>(h);
}

Process::Process(Process&& other)
    : Object(),
      rate_(0.0),
      signal_(false),
      shape_(),
      pdf_(nullptr),
      data_(nullptr),
      norm_(nullptr) {
  swap(*this, other);
}

Process& Process::operator=(Process other) {
  swap(*this, other);
  return (*this);
}

void Process::set_shape(std::unique_ptr<TH1> shape, bool set_rate) {
  // We were given a nullptr - this is fine, and so we're done
  if (!shape) {
    // This will safely release any existing TH1 held by shape_
    shape_ = nullptr;
    return;
  }
  // Otherwise we validate this new hist. First is to check that all bins have
  // +ve values, otherwise the interpretation as a pdf is tricky
  // for (int i = 1; i <= shape->GetNbinsX(); ++i) {
  //   if (shape->GetBinContent(i) < 0.) {
  //     throw std::runtime_error(FNERROR("TH1 has a bin with content < 0"));
  //   }
  // }
  // At this point we can safely move the shape in and take ownership
  shape_ = std::move(shape);
  // Ensure that root will not try and clean this up
  shape_->SetDirectory(0);
  if (set_rate) {
    this->set_rate(shape_->Integral());
  }
  if (shape_->Integral() > 0.) shape_->Scale(1. / shape_->Integral());
}

std::unique_ptr<TH1> Process::ClonedShape() const {
  if (!shape_) return std::unique_ptr<TH1>();
  std::unique_ptr<TH1> res(static_cast<TH1 *>(shape_->Clone()));
  res->SetDirectory(0);
  return res;
}

std::unique_ptr<TH1> Process::ClonedScaledShape() const {
  if (!shape_) return std::unique_ptr<TH1>();
  std::unique_ptr<TH1> res(ClonedShape());
  res->Scale(this->no_norm_rate());
  return res;
}

TH1F Process::ShapeAsTH1F() const {
  if (!shape_ && !data_) {
    throw std::runtime_error(
        FNERROR("Process object does not contain a shape"));
  }
  TH1F res;
  if (this->shape()) {
    // Need to get the shape as a concrete type (TH1F or TH1D)
    // A nice way to do this is just to use TH1D::Copy into a fresh TH1F
    TH1F const* test_f = dynamic_cast<TH1F const*>(this->shape());
    TH1D const* test_d = dynamic_cast<TH1D const*>(this->shape());
    if (test_f) {
      test_f->Copy(res);
    } else if (test_d) {
      test_d->Copy(res);
    } else {
      throw std::runtime_error(FNERROR("TH1 shape is not a TH1F or a TH1D"));
    }
  } else if (this->data()) {
    std::string var_name = this->data()->get()->first()->GetName();
    TH1F *tmp = dynamic_cast<TH1F*>(this->data()->createHistogram(
                           var_name.c_str()));
    res = *tmp;
    delete tmp;
    if (res.Integral() > 0.) res.Scale(1. / res.Integral());
  }
  return res;
}

std::ostream& Process::PrintHeader(std::ostream& out) {
  std::string line =
      (boost::format(
           "%-6s %-9s %-6s %-8s %-28s %-3i %-16s %-4i %-10.5g %-5i") %
       "mass" % "analysis" % "era" % "channel" % "bin" % "id" % "process" %
       "sig" % "rate" % "shape").str();
  std::string div(line.length(), '-');
  out << div  << std::endl;
  out << line << std::endl;
  out << div  << std::endl;
  return out;
}

std::ostream& operator<< (std::ostream &out, Process &val) {
  out << boost::format(
             "%-6s %-9s %-6s %-8s %-28s %-3i %-16s %-4i %-10.5g %-5i") %
             val.mass() % val.analysis() % val.era() % val.channel() %
             val.bin() % val.bin_id() % val.process() % val.signal() %
             val.rate() %
             (bool(val.shape()) || bool(val.pdf()) || bool(val.data()));
  return out;
}
}
