/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef BYTE_TAG_LIST_H
#define BYTE_TAG_LIST_H

#include <stdint.h>
#include "ns3/type-id.h"
#include "tag-buffer.h"

namespace ns3 {

struct ByteTagListData;

/**
 * \ingroup packet
 *
 * \brief keep track of the byte tags stored in a packet.
 *
 * This class is mostly private to the Packet implementation and users
 * should never have to access it directly.
 *
 * \internal
 * The implementation of this class is a bit tricky so, there are a couple
 * of things to keep in mind here:
 *
 *   - It stores all tags in a single byte buffer: each tag is stored
 *     as 4 32bit integers (TypeId, tag data size, start, end) followed 
 *     by the tag data as generated by Tag::Serialize.
 *
 *   - The struct ByteTagListData structure which contains the tag byte buffer
 *     is shared and, thus, reference-counted. This data structure is unshared
 *     as-needed to emulate COW semantics.
 *
 *   - Each tag tags a unique set of bytes identified by the pair of offsets
 *     (start,end). These offsets are provided by Buffer::GetCurrentStartOffset
 *     and Buffer::GetCurrentEndOffset which means that they are relative to 
 *     the start of the 'virtual byte buffer' as explained in the documentation
 *     for the ns3::Buffer class. Whenever the origin of the offset of the Buffer
 *     instance associated to this ByteTagList instance changes, the Buffer class
 *     reports this to its container Packet class as a bool return value
 *     in Buffer::AddAtStart and Buffer::AddAtEnd. In both cases, when this happens
 *     the Packet class calls ByteTagList::AddAtEnd and ByteTagList::AddAtStart to update
 *     the byte offsets of each tag in the ByteTagList.
 *
 *   - Whenever bytes are removed from the packet byte buffer, the ByteTagList offsets
 *     are never updated because we rely on the fact that they will be updated in
 *     either the next call to Packet::AddHeader or Packet::AddTrailer or when
 *     the user iterates the tag list with Packet::GetTagIterator and 
 *     TagIterator::Next.
 */
class ByteTagList
{
public:
  class Iterator
  {
  public:
    struct Item 
    {
      TypeId tid;
      uint32_t size;
      int32_t start;
      int32_t end;
      TagBuffer buf;
      Item (TagBuffer buf);
    private:
      friend class ByteTagList;
      friend class ByteTagList::Iterator;
    };
    bool HasNext (void) const;
    struct ByteTagList::Iterator::Item Next (void);
    uint32_t GetOffsetStart (void) const;
  private:
    friend class ByteTagList;
    Iterator (uint8_t *start, uint8_t *end, int32_t offsetStart, int32_t offsetEnd);
    void PrepareForNext (void);
    uint8_t *m_current;
    uint8_t *m_end;
    int32_t m_offsetStart;
    int32_t m_offsetEnd;
    uint32_t m_nextTid;
    uint32_t m_nextSize;
    int32_t m_nextStart;
    int32_t m_nextEnd;
  };

  ByteTagList ();
  ByteTagList (const ByteTagList &o);
  ByteTagList &operator = (const ByteTagList &o);
  ~ByteTagList ();

  /**
   * \param tid the typeid of the tag added
   * \param bufferSize the size of the tag when its serialization will 
   *        be completed. Typically, the return value of Tag::GetSerializedSize
   * \param start offset which uniquely identifies the first byte tagged by this tag.
   * \param end offset which uniquely identifies the last byte tagged by this tag.
   * \returns a buffer which can be used to write the tag data.
   *
   * 
   */
  TagBuffer Add (TypeId tid, uint32_t bufferSize, int32_t start, int32_t end);

  /**
   * \param o the other list of tags to aggregate.
   *
   * Aggregate the two lists of tags.
   */
  void Add (const ByteTagList &o);

  void RemoveAll (void);

  /**
   * \param offsetStart the offset which uniquely identifies the first data byte 
   *        present in the byte buffer associated to this ByteTagList.
   * \param offsetEnd the offset which uniquely identifies the last data byte 
   *        present in the byte buffer associated to this ByteTagList.
   * \returns an iterator
   *
   * The returned iterator will allow you to loop through the set of tags present
   * in this list: the boundaries of each tag as reported by their start and
   * end offsets will be included within the input offsetStart and offsetEnd.
   */
  ByteTagList::Iterator Begin (int32_t offsetStart, int32_t offsetEnd) const;

  /**
   * Adjust the offsets stored internally by the adjustment delta and
   * make sure that all offsets are smaller than appendOffset which represents
   * the location where new bytes have been added to the byte buffer.
   */
  void AddAtEnd (int32_t adjustment, int32_t appendOffset);
  /**
   * Adjust the offsets stored internally by the adjustment delta and
   * make sure that all offsets are bigger than prependOffset which represents
   * the location where new bytes have been added to the byte buffer.
   */
  void AddAtStart (int32_t adjustment, int32_t prependOffset);

private:
  bool IsDirtyAtEnd (int32_t appendOffset);
  bool IsDirtyAtStart (int32_t prependOffset);
  ByteTagList::Iterator BeginAll (void) const;

  struct ByteTagListData * Allocate (uint32_t size);
  void Deallocate (struct ByteTagListData *data);

  uint16_t m_used;
  struct ByteTagListData *m_data;
};

} // namespace ns3

#endif /* BYTE_TAG_LIST_H */
