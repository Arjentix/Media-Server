/*
MIT License

Copyright (c) 2021 Polyakov Daniil Alexandrovich

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <memory>
#include <vector>

#include "observer.h"

/**
 * @brief  Provider class, that can provide data to its observers
 *
 * @tparam Data Type of data to be provided
 */
template <typename Data>
class Provider {
 public:
  using SameDataObserver = Observer<Data>;

  virtual ~Provider() = default;

  /**
   * @brief Add new observer
   * @param observer_ptr Pointer to observer to be added
   */
  void AddObserver(std::shared_ptr<SameDataObserver> observer_ptr) {
    observers_.push_back(observer_ptr);
  }

 protected:
  /**
   * @brief Notify all observers
   * @param data Data to send to observers
   */
  void ProvideToAll(const Data &data) {
    for (auto observer_ptr : observers_) {
      observer_ptr->Receive(data);
    }
  }

 private:
  //! Vector of all observers
  std::vector<std::shared_ptr<SameDataObserver>> observers_;
};
