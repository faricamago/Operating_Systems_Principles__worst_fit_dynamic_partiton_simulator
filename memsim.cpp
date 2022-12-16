/// -------------------------------------------------------------------------------------
/// this is the only file you need to edit
/// -------------------------------------------------------------------------------------
///
/// (c) 2022, Pavol Federl, pfederl@ucalgary.ca
/// Do not distribute this file.

#include "memsim.h"
#include <cassert>
#include <iostream>
#include <unordered_map>
#include <iterator>
#include <list>
#include <set>
#include <math.h>


// from the A6-F22 document
struct Partition
{
  int tag;
  int64_t size, addr;
};

// from the A6-F22 document
typedef std::list<Partition>::iterator PartitionRef;

struct scmp
{
  bool operator()(const PartitionRef & c1, const PartitionRef & c2) const
  {
    if(c1->size == c2->size)
      return c1->addr < c2->addr;
    else
      return c1->size > c2->size;
  }
};

typedef std::set<PartitionRef>::iterator counter_tree;

// I recommend you implement the simulator as a class. This is only a suggestion.
// If you decide not to use this class, feel free to remove it.
struct Simulator
{
  //All partitions, in a linked list
  std::list<Partition> all_blocks;
  //Quick access to all tagged partitions
  std::unordered_map<long, std::vector<PartitionRef>> tagged_blocks;
  //Sorted partitions by size/address
  std::set<PartitionRef,scmp> free_blocks;
  PartitionRef temp;
  int64_t page_req;  
  int64_t size_of_page;
  Simulator(int64_t size_of_page)
  {
    // constructor
    this->page_req = 0;
    this->size_of_page = size_of_page;
    
  }
  void allocate(int tag, int size)
  {
    // Pseudocode for allocation request:
    // - search through the list of partitions from start to end, and
    //   find the largest partition that fits requested size
    //     - in case of ties, pick the first partition found
    // - if no suitable partition found:
    //     - get minimum number of pages from OS, but consider the
    //       case when last partition is free
    //     - add the new memory at the end of partition list
    //     - the last partition will be the best partition
    // - split the best partition in two if necessary
    //     - mark the first partition occupied, and store the tag in it
    //     - mark the second partition free
    bool is_empty_or_not = all_blocks.empty();
    if(is_empty_or_not)
    {
      int64_t new_size = this->size_of_page;
      int64_t ssize = ceil((double)size/(double)new_size);
      page_req = (page_req + ssize);
      new_size = this->size_of_page;
      new_size =  (int)ceil((double)size/(double)new_size)*new_size;
      all_blocks.push_back(Partition{-1,new_size, 0});
      free_blocks.insert(all_blocks.begin());
    }

    //largest free parition
    PartitionRef free_largest_part;
    bool worst_fit_not_found = true;
    is_empty_or_not = free_blocks.empty();
    if(!is_empty_or_not)
    {
      auto counter_tree = free_blocks.begin();
      if((*counter_tree)->size >= size )
      {
        if((*counter_tree)->tag == -1)
        {
         free_largest_part = *counter_tree;
         worst_fit_not_found = !(true);
        }
      }
    }

    
    if(worst_fit_not_found)
    { 
      int64_t new_size = this->size_of_page;
      int new_tag = std::prev(all_blocks.end())->tag;
      int64_t temp_size;
      if(new_tag != -1)
      {
        int64_t new_size2 = ceil((double)size/(double)new_size);
        temp_size = new_size2*new_size;
        page_req = page_req + ceil((double)size/(double)new_size);
        new_size2 = std::prev(all_blocks.end())->addr + std::prev(all_blocks.end())->size;
        all_blocks.push_back(Partition{-1, temp_size, new_size2});
        
      }
      else
      {
        free_blocks.erase(std::prev(all_blocks.end()));
        int64_t new_size2 = ((double)size-(double)std::prev(all_blocks.end())->size);
        temp_size = ceil(new_size2/(double)new_size)*new_size;
        new_size2 = ((double)size-(double)std::prev(all_blocks.end())->size);
        page_req += ceil(new_size2/(double)new_size);
        std::prev(all_blocks.end())->size = std::prev(all_blocks.end())->size + temp_size;
      }
      PartitionRef partt = std::prev(all_blocks.end());
      free_largest_part = partt;
      free_blocks.insert(partt);
    }

    int64_t new_size = free_largest_part->size;
    if(new_size<= size)
    {
      free_blocks.erase(free_largest_part);
      free_largest_part->tag = tag;
      tagged_blocks[tag].push_back(free_largest_part);
      free_largest_part->size = size;
    }
    else
    {
      free_blocks.erase(free_largest_part);
      new_size = free_largest_part->size-size;
      free_largest_part->tag = tag;
      tagged_blocks[tag].push_back(free_largest_part);
      free_largest_part->size = size;
      PartitionRef partt2 = std::prev(all_blocks.end());
      if(free_largest_part != partt2)
      {
        int64_t new_size3 = free_largest_part->addr+free_largest_part->size;
        partt2 = std::next(free_largest_part);
        all_blocks.insert(partt2, Partition{-1, new_size, new_size3});
        partt2 = std::next(free_largest_part);
        free_blocks.insert(partt2);
      }
      else
      {        
        int64_t new_size3 = free_largest_part->addr+free_largest_part->size;
        all_blocks.push_back(Partition{-1, new_size, new_size3});
        partt2 = std::prev(all_blocks.end());
        free_blocks.insert(partt2);
      }
    }
  }
  void deallocate(int tag)
  {
    // Pseudocode for deallocation request:
    // - for every partition
    //     - if partition is occupied and has a matching tag:
    //         - mark the partition free
    //         - merge any adjacent free partitions
    std::vector<PartitionRef> temp_vector = tagged_blocks[tag];
    for(auto j:temp_vector)
    {
      temp = j;
      temp->tag = -1;  

      PartitionRef previous = std::prev(all_blocks.end());
      PartitionRef nextt = std::next(temp);
      if(temp!= previous)
      {
        if(nextt->tag==-1)
        {
          nextt =  std::next(temp);
          free_blocks.erase(free_blocks.find(nextt));
          temp->size = temp->size + std::next(temp)->size;
          all_blocks.erase(std::next(temp));
        }
      }
      nextt = all_blocks.begin();
      previous = std::prev(temp);
      if(temp!= nextt)
      {
        if(previous->tag==-1)
        {
          free_blocks.erase(free_blocks.find(std::prev(temp)));
          temp->size= temp->size + std::prev(temp)->size;
          temp->addr = std::prev(temp)->addr;
          all_blocks.erase(std::prev(temp));
        }
      }
      
      free_blocks.insert(temp);
    }
    tagged_blocks.erase(tag);
  }
  
  void check_consistency()
  {
    // mem_sim() calls this after every request to make sure all data structures
    // are consistent. Since this will probablly slow down your code, you should
    // disable comment out the call below before submitting your code for grading.
    check_consistency_internal();
  }
  void check_consistency_internal()
  {
    // you do not need to implement this method at all - this is just my suggestion
    // to help you with debugging

    // here are some suggestions for consistency checks (see appendix also):

    // make sure the sum of all partition sizes in your linked list is
    // the same as number of page requests * size_of_page

    // make sure your addresses are correct

    // make sure the number of all partitions in your tag data structure +
    // number of partitions in your free blocks is the same as the size
    // of the linked list

    // make sure that every free partition is in free blocks

    // make sure that every partition in free_blocks is actually free

    // make sure that none of the partition sizes or addresses are < 1
  }
  MemSimResult getStats()
  {
    // let's guess the result... :)
    MemSimResult result;
    result.max_free_partition_size = 0;
    result.max_free_partition_address = 0;
    result.n_pages_requested = 0;
    result.n_pages_requested = page_req;
    int max_size = result.max_free_partition_size;
    for(auto i:all_blocks)
    {
      max_size = result.max_free_partition_size;
      if(i.tag == -1 && i.size > max_size)
      {
        result.max_free_partition_size = i.size;
        result.max_free_partition_address = i.addr;
      }
    }
    return result;
  }
};

// re-implement the following function
// ===================================
// parameters:
//    page_size: integer in range [1..1,000,000]
//    requests: array of requests
// return:
//    some statistics at the end of simulation
MemSimResult mem_sim(int64_t page_size, const std::vector<Request> & requests)
{
  // if you decide to use the simulator class above, you probably do not need
  // to modify below at all
  Simulator sim(page_size);
  for (const auto & req : requests)
  {
    if (req.tag < 0)
    {
      sim.deallocate(-req.tag);
    } else 
    {
      sim.allocate(req.tag, req.size);
    }
    sim.check_consistency();
  }
  return sim.getStats();
}
//end of program
