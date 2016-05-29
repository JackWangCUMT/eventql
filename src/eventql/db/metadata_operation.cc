/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include "eventql/db/metadata_operation.h"
#include "eventql/util/random.h"

namespace eventql {

MetadataOperation::MetadataOperation() {}

MetadataOperation::MetadataOperation(
    const String& db_namespace,
    const String& table_id,
    MetadataOperationType type,
    const SHA1Hash& input_txid,
    const SHA1Hash& output_txid,
    const Buffer& opdata) {
  data_.set_db_namespace(db_namespace);
  data_.set_table_id(table_id);
  data_.set_input_txid(input_txid.data(), input_txid.size());
  data_.set_output_txid(output_txid.data(), input_txid.size());
  data_.set_optype(type);
  data_.set_opdata(opdata.data(), opdata.size());
}

SHA1Hash MetadataOperation::getInputTransactionID() const {
  return SHA1Hash(data_.input_txid().data(), data_.input_txid().size());
}

SHA1Hash MetadataOperation::getOutputTransactionID() const {
  return SHA1Hash(data_.output_txid().data(), data_.output_txid().size());
}

Status MetadataOperation::decode(InputStream* is) {
  return Status(eRuntimeError, "not yet implemented");
}

Status MetadataOperation::encode(OutputStream* os) const {
  return Status(eRuntimeError, "not yet implemented");
}

Status MetadataOperation::perform(
    const MetadataFile& input,
    Vector<MetadataFile::PartitionMapEntry>* output) const {
  switch (data_.optype()) {
    case METAOP_BACKFILL_ADD_SERVER:
      return performBackfillAddServer(input, output);
    case METAOP_BACKFILL_REMOVE_SERVER:
      return performBackfillRemoveServer(input, output);
    default:
      return Status(eIllegalArgumentError, "invalid metadata operation type");
  }
}

Status MetadataOperation::performBackfillAddServer(
    const MetadataFile& input,
    Vector<MetadataFile::PartitionMapEntry>* output) const {
  if (input.getKeyspaceType() != KEYSPACE_UINT64) {
    return Status(eIllegalArgumentError, "keyspace type must be uint64");
  }

  auto opdata = msg::decode<BackfillAddServerOperation>(
      data_.opdata().data(),
      data_.opdata().size());

  uint64_t partition_begin;
  if (opdata.keyrange_begin().size() != sizeof(uint64_t)) {
    return Status(eIllegalArgumentError, "invalid keyrange begin");
  }

  memcpy(
      &partition_begin,
      opdata.keyrange_begin().data(),
      sizeof(uint64_t));

  auto pmap = input.getPartitionMap();

  MetadataFile::PartitionMapEntry* entry = nullptr;
  for (auto& e : pmap) {
    uint64_t e_begin;
    memcpy(&e_begin, e.begin.data(), sizeof(uint64_t));

  }

  auto iter = pmap.begin();
  while (iter != pmap.end()) {
    uint64_t e_begin;
    memcpy(&e_begin, iter->begin.data(), sizeof(uint64_t));

    if (e_begin == partition_begin) {
      entry = &*iter;
      break;
    }

    if (e_begin > partition_begin) {
      break;
    }
  }

  if (!entry) {
    MetadataFile::PartitionMapEntry e;
    e.begin = opdata.keyrange_begin();
    e.partition_id = SHA1Hash(
        opdata.partition_id().data(),
        opdata.partition_id().size());

    entry = &*pmap.insert(iter, e);
  }

  bool server_already_exists = false;
  for (const auto& s : entry->servers) {
    if (s.server_id == opdata.server_id()) {
      server_already_exists = true;
      break;
    }
  }

  if (!server_already_exists) {
    MetadataFile::PartitionPlacement s;
    s.server_id = opdata.server_id();
    s.placement_id = Random::singleton()->random64();
    entry->servers.emplace_back(s);
  }

  *output = pmap;
  return Status::success();
}

Status MetadataOperation::performBackfillRemoveServer(
    const MetadataFile& input,
    Vector<MetadataFile::PartitionMapEntry>* output) const {
  return Status(eRuntimeError, "operation not implemented");
}

} // namespace eventql

