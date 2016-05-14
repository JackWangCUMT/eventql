/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <eventql/util/option.h>
#include <eventql/sql/backends/backend.h>
#include <eventql/sql/backends/tableref.h>
#include <eventql/sql/TableInfo.h>
#include <eventql/sql/table_provider.h>

namespace csql {
class QueryBuilder;
class Transaction;
class SequentialScanNode;

using namespace stx;

class TableRepository : public TableProvider {
public:

  virtual TableRef* getTableRef(const std::string& table_name) const;

  void addTableRef(
      const std::string& table_name,
      std::unique_ptr<TableRef>&& table_ref);

  void import(
      const std::vector<std::string>& tables,
      const std::string& source_uri,
      const std::vector<std::unique_ptr<Backend>>& backends);

  void addProvider(RefPtr<TableProvider> provider);

  void listTables(Function<void (const TableInfo& table)> fn) const override;

  Option<TableInfo> describe(const String& table_name) const override;

  Option<ScopedPtr<TableExpression>> buildSequentialScan(
      Transaction* ctx,
      RefPtr<SequentialScanNode> seqscan) const override;

protected:
  std::unordered_map<std::string, std::unique_ptr<TableRef>> table_refs_;

  Vector<RefPtr<TableProvider>> providers_;
};

} // namespace csql