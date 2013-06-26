/*!
 * \file flame2/mem/memory_iterator.hpp
 * \author Shawn Chin
 * \date 2012
 * \copyright Copyright (c) 2012 STFC Rutherford Appleton Laboratory
 * \copyright Copyright (c) 2012 University of Sheffield
 * \copyright GNU Lesser General Public License
 * \brief Memory Iterator
 */
#ifndef MEM__MEMORY_ITERATOR_HPP_
#define MEM__MEMORY_ITERATOR_HPP_
#include <string>
#include <utility>
#include <map>
#include "flame2/exceptions/mem.hpp"
#include "agent_shadow.hpp"
#include "vector_wrapper.hpp"

namespace flame { namespace mem {

//! Map of anonymous pointers
typedef std::map<std::string, void*> VoidPtrMap;
//! Type of element stored in VoidPtrMap
typedef VoidPtrMap::value_type VoidPtrMapValue;

//! Agent memory iterator
class MemoryIterator {
  friend class AgentShadow;

  public:
    //! Resets position to beginning of agent data
    void Rewind();

    //! Progresses all pointers to point to next set of data
    //! \return true if progress was made, false if already at end of iteration
    bool Step();

    //! Query if iteration has completed
    //! \return true if iteration has completed
    bool AtEnd() const;

    //! Query the population size
    //! \return population size
    size_t get_size() const;

    //! Query the iteration offset
    //! \return iteration offset
    size_t get_offset() const;

    //! Query the iteration count
    //! \return iteration count
    size_t get_count() const;

    //! Query the number of steps taken so far
    //! \return position within current iteration
    size_t get_position() const;

    //! Returns a const pointer to the actual data location
    //! \param var_name variable name
    //! \return const pointer to underlying data
    template <typename T>
    const T* GetReadPtr(const std::string& var_name) const {
      try {
#ifndef DISABLE_RUNTIME_TYPE_CHECKING
        VectorWrapperBase* vwb = vec_map_ptr_->at(var_name);
        if (*(vwb->GetDataType()) != typeid(T)) {
          throw flame::exceptions::invalid_type("invalid type");
        }
#endif
        return static_cast<const T* const>(ptr_map_.at(var_name));
      }
      catch(const std::out_of_range& E) {
        if (!shadow_->IsRegistered(var_name)) {
          throw flame::exceptions::invalid_variable("invalid variable");
        } else {
          throw flame::exceptions::invalid_operation("no read access to var");
        }
      }
    }

    //! Returns a pointer to the actual data location
    //! \param var_name variable name
    //! \return pointer to underlying data
    template <typename T>
    T* GetWritePtr(const std::string& var_name) const {
      if (rw_set_ptr_->find(var_name) == rw_set_ptr_->end()) {
        if (!shadow_->IsRegistered(var_name)) {
          throw flame::exceptions::invalid_variable("invalid variable");
        } else {
          throw flame::exceptions::invalid_operation("no write access to var");
        }
      }

#ifndef DISABLE_RUNTIME_TYPE_CHECKING
      VectorWrapperBase* vwb = vec_map_ptr_->at(var_name);
      if (*(vwb->GetDataType()) != typeid(T)) {
        throw flame::exceptions::invalid_type("invalid type");
      }
#endif
      return static_cast<T* const>(ptr_map_.at(var_name));
    }

    //! Returns the value of a given variable
    //! \param var_name variable name
    //! \return value of current variable
    template <typename T>
    T Get(const std::string& var_name) const {
      const T* ptr = GetReadPtr<T>(var_name);
#ifndef NDEBUG
      if (ptr == NULL) {
        throw flame::exceptions::out_of_range("end of iterator met");
      }
#endif
      return *(ptr);
    }

    //! Sets the value of a given variable
    //! \param var_name variable name
    //! \param value value to set variable to
    template <typename T>
    void Set(const std::string& var_name, T value) {
      T* ptr = GetWritePtr<T>(var_name);
#ifndef NDEBUG
      if (ptr == NULL) {
        throw flame::exceptions::out_of_range("end of iterator met");
      }
#endif
      *(ptr) = value;
    }

  protected:
    //! Protected constructor. Access limited to AgentShadow
    //! \param shadow pointer to parent agent shadow
    explicit MemoryIterator(AgentShadow* shadow);

    //! Protected constructor. Access limited to AgentShadow
    //! \param shadow pointer to parent agent shadow
    //! \param offset memory offset
    //! \param count number of elements to be addressed by the iterator
    MemoryIterator(AgentShadow* shadow, size_t offset, size_t count);

  private:
    size_t position_;  //!< Current iterator position
    size_t size_;  //!< Population size
    size_t offset_;  //!< Offset to start iterating from
    size_t count_;  //!< Number or elements to iterate through
    AgentShadow* shadow_;  //!< Pointer to agent shadow instance
    VoidPtrMap ptr_map_;  //!< map of raw pointers of vars
    ConstVectorMap* vec_map_ptr_;  //!< pointer to vec map
    WriteableSet* rw_set_ptr_;  //!< Pointer to set of writeable vars
};

}}  //  namespace flame::mem


#endif  // MEM__MEMORY_ITERATOR_HPP_
