/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * Licensed under the MIT license (see LICENSE).
 */
#ifndef _FNORDMETRIC_MEMORYBACKEND_H
#define _FNORDMETRIC_MEMORYBACKEND_H

#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <memory>
#include <unordered_map>
#include "storagebackend.h"
#include "clock.h"

namespace fnordmetric {

/**
 * Memory Storage Backend for testing
 *
 * This will allocate new memory for ever record and never free any of it until
 * destroyed. Also the implementation of seek is O(n). Don't use this except for
 * testing purposes!
 *
 * See "storagebackend.h" for extended documentation.
 */
class MemoryBackend : public IStorageBackend {
public:
  class Cursor : public IStorageCursor {
  public:

    Cursor(const std::shared_ptr<std::vector<IStorageCursor::RowType>>& data) :
        data_(data),
        pos_(-1) {}

    virtual uint64_t seekTo(uint64_t position) override {
      // FIXPAUL implement me
    }

    virtual uint64_t seekToFirst() override {
      if (data_->size() > 0) {
        pos_ = 0;
        return std::get<0>(data_->at(pos_));
      } else {
        return 0;
      }
    }

    virtual uint64_t seekToLast() override {
      if (data_->size() > 0) {
        pos_ = data_->size() - 1;
        return std::get<0>(data_->at(pos_));
      } else {
        return 0;
      }
    }

    typedef
        std::function<void (const uint8_t* data, size_t len, uint64_t time)>
        GetRowCB;

    virtual void getRow(const GetRowCB& func) const override {
      if (pos_ < 0 || pos_ >= data_->size()) {
        return;
      }

      auto row = data_->at(pos_);
      auto data = std::get<1>(row).data();
      auto len = std::get<1>(row).size();
      auto time = std::get<0>(row);

      func(data, len, time);
    }

    virtual bool next() {
      return false;
    }

    virtual uint64_t appendRow(const std::vector<uint8_t>& data) override {
      uint64_t now = WallClock::getUnixMillis();
      data_->emplace_back(std::make_pair(now, data));
      printf("append_row time=%llu, size=%lu\n", now, data.size());
      return now;
    }

    virtual std::unique_ptr<IStorageCursor> clone() const override {
      return std::unique_ptr<IStorageCursor>(new Cursor(data_, pos_));
    }

  protected:
    Cursor(
        const std::shared_ptr<std::vector<IStorageCursor::RowType>>& data,
        int pos) :
        data_(data),
        pos_(pos) {}

    const std::shared_ptr<std::vector<IStorageCursor::RowType>> data_;
    int pos_;
  };

  MemoryBackend() : IStorageBackend() {}
  MemoryBackend(const MemoryBackend& copy) = delete;

  static std::unique_ptr<MemoryBackend> New() {
    return std::unique_ptr<MemoryBackend>(new MemoryBackend());
  }

  virtual std::unique_ptr<IStorageCursor> getCursor(
      const std::string& key) override {
    std::unique_ptr<IStorageCursor> cursor_ptr;
    auto iter = streams_.find(key);

    if (iter == streams_.end()) {
      std::shared_ptr<std::vector<IStorageCursor::RowType>> data_ptr(
        new std::vector<IStorageCursor::RowType>());

      streams_.insert(std::make_pair(key, data_ptr));
      cursor_ptr.reset(new Cursor(data_ptr));
    } else {
      cursor_ptr.reset(new Cursor(iter->second));
    }

    return cursor_ptr;
  }

protected:
  std::unordered_map<
      std::string,
      std::shared_ptr<std::vector<IStorageCursor::RowType>>> streams_;
};

}

#endif
