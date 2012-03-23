/* The MIT License:

Copyright (c) 2008-2012 Ivan Gagis <igagis@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. */

// Home page: http://ting.googlecode.com



/**
 * @file Ref.hpp
 * @author Ivan Gagis <igagis@gmail.com>
 * @brief Reference counting mechanism base classes.
 */

#pragma once

#include "debug.hpp"
#include "types.hpp"
#include "atomic.hpp"
#include "PoolStored.hpp"
#include "Thread.hpp"

//#define M_ENABLE_REF_PRINT
#ifdef M_ENABLE_REF_PRINT
#define M_REF_PRINT(x) TRACE(<<"[REF]" x)
#else
#define M_REF_PRINT(x)
#endif

namespace ting{

template <class T> class Ref;//forward declaration
template <class T> class WeakRef;//forward declaration

/**
 * @brief base class for a reference counted object.
 * All object which are supposed to be used with reference counted pointers (ting::Ref)
 * should be derived from ting::RefCounted.
 * Typical usage:
 * @code
 *	class Test : public ting::RefCounted{
 *		Test(int theA, int theB) :
 *				a(theA),
 *				b(theB)
 *		{}
 *	public:
 *		int a;
 *		int b;
 *
 *		static inline ting::Ref<Test> New(int theA, int theB){
 *			return ting::Ref<Test>(new Test(theA, theB));
 *		}
 *	}
 *
 *	//...
 *
 *	{
 *		ting::Ref<Test> t = Test::New(10, -13);
 * 
 *		t->a = 23;
 *		//...
 *
 *		ting::Ref<Test> t2 = t;
 *		t2->b = 45;
 *
 *		//...
 *
 *	}//the object will be destroyed here
 * @endcode
 *
 * Note, the constructor of class Test in this example was done private and static method New() introduced
 * to construct objects of class Test. It is recommended to do this for all ting::RefCounted
 * objects because they are not supposed to be accessed via ordinary pointers, only via ting::Ref.
 * It is a good practice and it will make your code less error prone.
 */
class RefCounted{
	template <class T> friend class Ref;
	template <class T> friend class WeakRef;


private:

	struct Counter{
		ting::atomic::S32 numStrongRefs;

		//WeakRef's are the references which control the life time of the Counter object.
		//I.e. WeakRef's are playing role of _strong_ references for Counter object.
		//Think of RefCounted object as additional _strong_ reference to the Counter object,
		//this is why number of WeakRef's is initialized to 1.
		ting::atomic::S32 numWeakRefs;

		inline Counter() :
				numWeakRefs(1)//1 because RefCounted acts as weak reference.
		{
			M_REF_PRINT(<< "Counter::Counter(): counter object created" << std::endl)
		}

		inline ~Counter()throw(){
			M_REF_PRINT(<< "Counter::~Counter(): counter object destroyed" << std::endl)
		}
		
		inline static void* operator new(size_t size){
			ASSERT(size == sizeof(Counter))

			return RefCounted::memoryPool.Alloc_ts();
		}

		inline static void operator delete(void *p)throw(){
			RefCounted::memoryPool.Free_ts(p);
		}
	};

	
	//Memory pool for Counter objects
	static ting::MemoryPool<sizeof(Counter), 512> memoryPool;

	Counter *counter;



protected:
	/**
	 * @brief Constructor.
	 * This class is only supposed to be as base class, this is why this
	 * constructor is protected.
	 */
	//only base classes can construct this class
	//i.e. use of this class is allowed only as a base class
	inline RefCounted(){
		//NOTE: do not create Counter object in RefCounted constructor
		//      initializer list because MSVC complains about usage of "this"
		//      keyword in initializer list.
		this->counter = new Counter();
	}



	/**
	 * @brief operator new.
	 * This operator new just calls and returns the result of global 'new'.
	 * This operator new is protected to enforce objects creation
	 * through static New() method, to avoid using of RefCounted objects
	 * via ordinary pointers. See description of the RefCounted class for more
	 * info.
	 * @param s - size of the memory block to allocate.
	 * @return address of the allocated memory block.
	 */
	inline static void* operator new(size_t s){
		return ::operator new(s);
	}

protected:
	/**
	 * @brief operator delete.
	 * Releases allocated memory.
	 * This operator delete is protected because only ting::Ref can delete the
	 * RefCounted object.
	 * NOTE: cannot make operator delete private because it should have same
	 *       access level as operator new, because in case of failure in
	 *       the constructor of the object (i.e. it throws an exception) the
	 *       allocated memory should be freed with "delete", thus when using
	 *       operator new it implicitly needs to be possible to call operator delete
	 *       from the same place where the operator new is used.
	 * @param p - address of the memory block to release.
	 */
	inline static void operator delete(void *p)throw(){
		::operator delete(p);
	}

	/**
	 * @brief Destructor.
	 * Destructor is protected because of the same reason as operator delete is.
	 */
	virtual ~RefCounted()throw(){
		//Remove reference to Counter object held by this RefCounted
		ASSERT(this->counter)

#ifdef DEBUG
		//since RefCounted is being destroyed, that means that there are no strong ref's left
		ASSERT(this->counter->numStrongRefs.FetchAndAdd(0) == 0)
{
		ting::s32 res = this->counter->numWeakRefs.FetchAndAdd(0);
		ASSERT_INFO(res >= 1, "res = " << res)
}
#endif

		if(this->counter->numWeakRefs.FetchAndAdd(-1) == 1){//there was only 1 weak ref
			delete this->counter;
		}
	}

public:
	/**
	 * @brief Returns current number of strong references pointing to this object.
	 * This method returns the current number of strong references.
	 * It is not guaranteed that the returned number is actual, since new references
	 * may be constructed and some may be destroyed meanwhile.
	 * One should not rely on the returned value unless he knows what he is doing!
	 * @return number of strong references.
	 */
	inline unsigned NumRefs()const throw(){
		ASSERT(this->counter)
		ting::s32 ret = this->counter->numStrongRefs.FetchAndAdd(0);
		ASSERT(ret > 0)
		return unsigned(ret);
	}

private:
	//copy constructor is private, no copying
	inline RefCounted(const RefCounted&);
	inline RefCounted& operator=(const RefCounted&);
};//~class RefCounted



/**
 * @brief Reference to a reference counted object.
 * Pointer (reference) to a reference counted object.
 * As soon as there is at least one ting::Ref object pointing to some reference counted
 * object, this object will be existing. As soon as all ting::Ref objects cease to exist
 * (going out of scope) the reference counted object they are pointing ti will be deleted.
 */
//T should be RefCounted!!!
template <class T> class Ref{
	friend class WeakRef<T>;
	template <class TS> friend class Ref;

	T *p;



public:
	/**
	 * @brief cast statically to another class.
	 * Performs standard C++ static_cast().
	 * @return reference to object of casted class.
	 */
	template <class TS> inline Ref<TS> StaticCast()const throw(){
		return Ref<TS>(static_cast<TS*>(this->operator->()));
	}



	/**
	 * @brief cast dynamically.
	 * Performs standard C++ dynamic_cast() operation.
	 * @return valid reference to object of casted class if dynamic_cast() succeeds, i.e. if the
	 *         object can be cast to requested class.
	 * @return invalid reference otherwise, i. e. if the object cannot be cast to requested class.
	 */
	template <class TS> inline const Ref<TS> DynamicCast()const throw(){
		const TS* t = dynamic_cast<const TS*>(this->operator->());
		if(t){
			return Ref<TS>(const_cast<TS*>(t));
		}else{
			return Ref<TS>();
		}
	}



	/**
	 * @brief default constructor.
	 * @param v - this parameter is ignored. Its intention is just to make possible
	 *            auto-conversion form int to invalid reference. This allows writing
	 *            simply 'return 0;' in functions to return invalid reference.
	 *            Note, any integer passed (even not 0) will result in invalid reference.
	 */
	//NOTE: the int argument is just to make possible
	//auto conversion from 0 to invalid Ref object
	//i.e. it will be possible to write 'return 0;'
	//from the function returning Ref
	inline Ref(int v = 0)throw() :
			p(0)
	{
		M_REF_PRINT(<< "Ref::Ref(): invoked, p=" << (this->p) << std::endl)
	}



	/**
	 * @brief construct reference to given object.
	 * Constructs a reference to a given reference counted object.
	 * Note, that it is supposed that first reference will be constructed
	 * right after object creation, and further work with object will only be done
	 * via ting::Ref references, not ordinary pointers.
	 * Note, that this constructor is explicit, this is done to prevent undesired
	 * automatic conversions from ordinary pointers to Ref.
	 * @param rc - ordinary pointer to ting::RefCounted object.
	 */
	//NOTE: this constructor should be explicit to prevent undesired conversions from T* to Ref<T>
	explicit inline Ref(T* rc)throw() :
			p(rc)
	{
		M_REF_PRINT(<< "Ref::Ref(rc): invoked, p = " << (this->p) << std::endl)
		ASSERT_INFO(this->p, "Ref::Ref(rc): rc is 0")

		//NOTE: in order to make sure that passed object inherits RefCounted
		//do static_cast() to RefCounted. Since the T type may be a const type
		//(e.g. Ref<const int>), so cast to const RefCounted and then use const_cast().
		if(this->p){
			const_cast<RefCounted*>(
					static_cast<const RefCounted*>(this->p)
				)->counter->numStrongRefs.FetchAndAdd(1);
		}
		M_REF_PRINT(<< "Ref::Ref(rc): exiting" << (this->p) << std::endl)
	}



	/**
	 * @brief Construct reference from weak reference.
	 * @param r - weak reference.
	 */
	inline Ref(const WeakRef<T> &r)throw();



private:
	template <class TS> inline void InitFromStrongRef(const Ref<TS>& r)throw(){
		this->p = r.p; //should downcast automatically
		if(this->p){
			//NOTE: first, static cast to const RefCounted, because even if T is a const type
			//it is possible to do. After that we can cast to non-const RefCounted and increment number of strong references.
			//Thus, we also ensure at compile time that T inherits RefCounted.
			const_cast<RefCounted*>(
					static_cast<const RefCounted*>(this->p)
				)->counter->numStrongRefs.FetchAndAdd(1);
		}
	}


public:
	/**
	 * @brief Copy constructor.
	 * Creates new reference object which refers to the same object as 'r'.
	 * @param r - existing Ref object to make copy of.
	 */
	//copy constructor
	inline Ref(const Ref& r)throw(){
		M_REF_PRINT(<< "Ref::Ref(copy): invoked, r.p = " << (r.p) << std::endl)
		this->InitFromStrongRef<T>(r);
	}



	/**
	 * @brief Constructor for automatic down-casting and const-casting.
	 * Normally, this constructor is not supposed to be used explicitly.
	 * @param r - strong reference to cast.
	 */
	//downcast / to-const cast constructor
	template <class TS> inline Ref(const Ref<TS>& r)throw(){
		M_REF_PRINT(<< "Ref::Ref(copy): invoked, r.p = " << (r.p) << std::endl)
		this->InitFromStrongRef<TS>(r);
	}



	/**
	 * @brief Destructor.
	 */
	inline ~Ref()throw(){
		M_REF_PRINT(<< "Ref::~Ref(): invoked, p = " << (this->p) << std::endl)
		this->Destroy();
	}



	/**
	 * @brief tells whether the reference is pointing to some object or not.
	 * @return true if reference is pointing to valid object.
	 * @return false if the reference does not point to any object.
	 */
	//returns true if the reference is valid (not 0)
	inline bool IsValid()const throw(){
		M_REF_PRINT(<<"Ref::IsValid(): invoked, this->p="<<(this->p)<<std::endl)
		return (this->p != 0);
	}



	/**
	 * @brief tells whether the reference is pointing to some object or not.
	 * Inverse of ting::Ref::IsValid().
	 * @return false if reference is pointing to valid object.
	 * @return true if the reference does not point to any object.
	 */
	inline bool IsNotValid()const throw(){
		M_REF_PRINT(<<"Ref::IsNotValid(): invoked, this->p="<<(this->p)<<std::endl)
		return !this->IsValid();
	}



	/**
	 * @brief tells if 2 references are equal.
	 * @param r - reference to compare this reference to.
	 * @return true if both references are pointing to the same object or both are invalid.
	 * @return false otherwise.
	 */
	template <class TS> inline bool operator==(const Ref<TS>& r)const throw(){
		return this->p == r.p;
	}



	/**
	 * @brief tells if 2 references are not equal.
	 * @param r - reference to compare this reference to.
	 * @return false if both references are pointing to the same object or both are invalid.
	 * @return true otherwise.
	 */
	template <class TS> inline bool operator!=(const Ref<TS>& r)const throw(){
		return !(this->operator==<TS>(r));
	}



	/**
	 * @brief Compares two references.
	 * The comparison is done the same way as it would be done for ordinary pointers.
	 * @param r - reference to compare to.
	 * @return true if the address of this object is less than the address of object referred by 'r'.
	 * @return false otherwise.
	 */
	template <class TS> inline bool operator<(const Ref<TS>& r)const throw(){
		return this->p < r.p;
	}



	/**
	 * @brief Compares two references.
	 * The comparison is done the same way as it would be done for ordinary pointers.
	 * @param r - reference to compare to.
	 * @return true if the address of this object is less or equal to the address of object referred by 'r'.
	 * @return false otherwise.
	 */
	template <class TS> inline bool operator<=(const Ref<TS>& r)const throw(){
		return this->p <= r.p;
	}



	/**
	 * @brief Compares two references.
	 * The comparison is done the same way as it would be done for ordinary pointers.
	 * @param r - reference to compare to.
	 * @return true if the address of this object is greater than the address of object referred by 'r'.
	 * @return false otherwise.
	 */
	template <class TS> inline bool operator>(const Ref<TS>& r)const throw(){
		return this->p > r.p;
	}



	/**
	 * @brief Compares two references.
	 * The comparison is done the same way as it would be done for ordinary pointers.
	 * @param r - reference to compare to.
	 * @return true if the address of this object is greater or equal to the address of object referred by 'r'.
	 * @return false otherwise.
	 */
	template <class TS> inline bool operator>=(const Ref<TS>& r)const throw(){
		return this->p >= r.p;
	}



	/**
	 * @brief tells if the reference is invalid.
	 * @return true if the reference is invalid.
	 * @return false if the reference is valid.
	 */
	inline bool operator!()const throw(){
		return !this->IsValid();
	}



	/**
	 * @brief A bool-like type.
	 * This type is used instead of bool for automatic conversion of reference to
	 * this bool-like type. The advantage of using this instead of ordinary bool type is
	 * that it prevents further undesired automatic conversions to, for example, int.
	 */
	typedef void (Ref::*unspecified_bool_type)();



	/**
	 * @brief tells if the reference is valid.
	 * This operator is a more type-safe version of conversion-to-bool operator.
	 * Usage of standard 'operator bool()' is avoided because it may lead to undesired
	 * automatic conversions to int and other types.
	 * It is intended to be used as follows:
	 * @code
	 *	ting::Ref r = TestClass::New();
	 *	if(r){
	 *		//r is valid.
	 *		ASSERT(r)
	 *		r->DoSomethig();
	 *	}else{
	 *		//r is invalid
	 *	}
	 * @endcode
	 */
	//NOTE: Safe conversion to bool type.
	//      Because if using simple "operator bool()" it may result in chained automatic
	//      conversion to undesired types such as int.
	inline operator unspecified_bool_type()const throw(){
		return this->IsValid() ? &Ref::Reset : 0;//Ref::Reset is taken just because it has matching signature
	}

	//NOTE: do not use this type of conversion, see NOTE to "operator unspecified_bool_type()"
	//      for details.
//	inline operator bool(){
//		return this->IsValid();
//	}



	/**
	 * @brief make this ting::Ref invalid.
	 * Resets this reference making it invalid and destroying the
	 * object it points to if necessary (if no references to the object left).
	 */
	void Reset()throw(){
		this->Destroy();
		this->p = 0;
	}



	/**
	 * @brief assign reference.
	 * Note, that if this reference was pointing to some object, the object will
	 * be destroyed if there are no other references. And this reference will be assigned
	 * a new value.
	 * @param r - reference to assign to this reference.
	 * @return reference to this Ref object.
	 */
	Ref& operator=(const Ref& r)throw(){
		M_REF_PRINT(<< "Ref::operator=(): invoked, p = " << (this->p) << std::endl)
		if(this == &r){
			return *this;//detect self assignment
		}
		
		this->Destroy();
		this->InitFromStrongRef<T>(r);
		return *this;
	}



	/**
	 * @brief Assignment operator.
	 * Assignment operator which also does automatic downcast and const cast if needed.
     * @param r - strong reference to assign from.
     * @return reference to this strong reference object.
     */
	//downcast / to-const assignment
	template <class TS> Ref<T>& operator=(const Ref<TS>& r)throw(){
		//self-assignment should be impossible
		ASSERT(reinterpret_cast<const void*>(this) != reinterpret_cast<const void*>(&r))

		this->Destroy();
		this->InitFromStrongRef<TS>(r);

		return *this;
	}



	/**
	 * @brief Create a weak reference.
	 * This is a convenience function which creates and returns a
	 * weak reference.
	 * @return weak reference created from this strong reference.
	 */
	inline WeakRef<T> GetWeakRef()const throw(){
		return WeakRef<T>(*this);
	}



	/**
	 * @brief operator *.
	 * Works the same way as operator * for ordinary pointers.
     * @return reference to RefCounted object pointed by this strong reference object.
     */
	//NOTE: the operator is const because const Ref does not mean that the object it points to cannot be changed,
	//it means that the Ref itself cannot be changed to point to another object.
	inline T& operator*()const throw(){
		M_REF_PRINT(<< "Ref::operator*(): invoked, p = " << (this->p) << std::endl)
		ASSERT_INFO(this->p, "Ref::operator*(): this->p is zero")
		return static_cast<T&>(*this->p);
	}



	/**
	 * @brief opearator->.
     * @return pointer to the RefCounted object pointed by this strong reference.
     */
	//NOTE: the operator is const because const Ref does not mean that the object it points to cannot be changed,
	//it means that the Ref itself cannot be changed to point to another object.
	inline T* operator->()const throw(){
		M_REF_PRINT(<< "Ref::operator->(): invoked, p = " << (this->p) << std::endl)
		ASSERT_INFO(this->p, "Ref::operator->(): this->p is zero")
		return this->p;
	}



private:
	inline void Destroy()throw(){
		if(this->IsValid()){
			ASSERT(this->p->counter)
			if(this->p->counter->numStrongRefs.FetchAndAdd(-1) == 1){//if there was only one strong ref
				M_REF_PRINT(<< "Ref::Destroy(): deleting " << (this->p) << std::endl)
				//deleting should be ok without type casting, because RefCounted
				//destructor is virtual.
				delete this->p;
				M_REF_PRINT(<< "Ref::Destroy(): object " << (this->p) << " deleted" << std::endl)
			}
		}
	}



	//Ref objects can only be created on stack
	//or as a member of other object or array,
	//thus, make "operator new" private.
	static void* operator new(size_t size);
};//~class Ref



/**
 * @brief Weak Reference to a reference counted object.
 * A weak reference to reference counted object. The object is destroyed as soon as
 * there are no references to the object, even if there are weak references.
 * Thus, weak reference can point to some object but pointing to that object cannot
 * prevent deletion of this object.
 * To work with the object one needs to construct a hard reference (ting::Ref) to that
 * object first, a hard reference can be constructed from weak reference:
 * @code
 *	ting::Ref<TestCalss> t = TestClass::New();
 *	
 *	ting::WeakRef<TestClass> weakt = t; //weakt points to object
 * 
 *	//...
 * 
 *	if(ting::Ref<TestClass> obj = weakt){
 *		//object still exists
 *		obj->DoSomething();
 *	}
 *
 *	t.Reset();//destroy the object
 *	//the object of class TestClass will be destroyed here,
 *	//despite there is a weak reference to that object.
 *	//From now on, the weak reference does not point to any object.
 *
 *	if(ting::Ref<TestClass> obj = weakt){
 *		//we will not get there
 *		ASSERT(false)
 *	}
 * @endcode
 */
//T should be RefCounted!!!
template <class T> class WeakRef{
	friend class Ref<T>;
	template <class TS> friend class WeakRef;

	RefCounted::Counter *counter;
	T* p;//this pointer is only valid if counter is not 0 and there are strong references in the counter



	inline void InitFromRefCounted(T *rc)throw(){
		M_REF_PRINT(<< "WeakRef::InitFromRefCounted(): invoked " << std::endl)
		if(!rc){
			this->counter = 0;
			return;
		}

		ASSERT(rc->counter)

		this->counter = rc->counter;
		this->p = rc;//should cast automatically
		//NOTE: if you get 'invalid conversion' error here, then you must be trying
		//to do automatic cast to unrelated type.

		//increment number of weak references
		{
			ting::s32 res = this->counter->numWeakRefs.FetchAndAdd(1);
			ASSERT_INFO(res >= 1, "res = " << res)//make sure there was at least one weak reference (RefCounted itself acts like a weak reference as well)
		}
	}



	inline void InitFromStrongRef(Ref<T> &r)throw(){
		M_REF_PRINT(<< "WeakRef::InitFromStrongRef(): invoked " << std::endl)

		//it is ok if 'r' is not valid, InitFromRefCounted() does check for zero pointer.
		this->InitFromRefCounted(r.p);
	}



	template <class TS> inline void InitFromWeakRef(const WeakRef<TS>& r)throw(){
		M_REF_PRINT(<< "WeakRef::InitFromWeakRef(): invoked " << std::endl)
		if(r.counter == 0){
			this->counter = 0;
			return;
		}
		ASSERT(r.counter)

		this->counter = r.counter;

		this->p = r.p;//should cast automatically
		//NOTE: if you get 'invalid conversion' error here, then you must be trying
		//to do automatic cast to unrelated type.

		//increment number of weak references
		{
			ting::s32 res = this->counter->numWeakRefs.FetchAndAdd(1);
			ASSERT(res >= 1)//make sure there was at least one weak reference (RefCounted itself acts like a weak reference as well)
		}
	}



	inline void Destroy()throw(){
		if(this->counter == 0){
			return;
		}

		if(this->counter->numWeakRefs.FetchAndAdd(-1) == 1){//if there was only 1 weak reference before decrement
			ASSERT(this->counter->numStrongRefs.FetchAndAdd(0) == 0)
			delete this->counter;
		}
	}



public:
	/**
	 * @brief make weak reference from pointer to RefCounted.
	 * This constructor makes a weak reference from ordinary pointer to a
	 * RefCounted object. Mainly, weak references should be constructed this way
	 * only from within the constructor of the class derived from RefCounted.
	 * This is because in constructor it is sometimes needed to create a weak
	 * reference to the object itself and there is no any strong references to
	 * the object yet because the object is still under construction.
	 * But, it is safe to create weak references already, since first strong
	 * reference should normally be created shortly after the object has
	 * finished construction.
	 * @param rc - ordinary pointer to a RefCounted object.
	 */
	inline WeakRef(T* rc = 0)throw(){
		M_REF_PRINT(<< "WeakRef::WeakRef(T*): invoked" << std::endl)
		this->InitFromRefCounted(rc);
	}



	/**
	 * @brief Constructor.
	 * Creates weak reference from strong reference.
     * @param r
     */
	inline WeakRef(const Ref<T> &r)throw(){
		M_REF_PRINT(<< "WeakRef::WeakRef(const Ref<T>&): invoked" << std::endl)
		this->InitFromStrongRef(const_cast<Ref<T>&>(r));
	}



	/**
	 * @brief Copy constructor.
     * @param r - weak reference to copy from.
     */
	inline WeakRef(const WeakRef& r)throw(){
		M_REF_PRINT(<< "WeakRef::WeakRef(const WeakRef&): invoked" << std::endl)
		this->InitFromWeakRef<T>(r);
	}



	/**
	 * @brief Constructor.
	 * Template constructor for automatic type down-casting and const-casting.
	 * Normally, this constructor should not be used explicitly.
     * @param r - weak reference to copy from.
     */
	//downcast / to-const cast constructor
	template <class TS> inline WeakRef(const WeakRef<TS>& r)throw(){
		M_REF_PRINT(<< "WeakRef::WeakRef(const WeakRef<TS>&): invoked" << std::endl)
		this->InitFromWeakRef<TS>(r);
	}



	/**
	 * @brief Get strong reference.
	 * This is just a convenience method which creates and returns a strong
	 * reference from this weak reference.
	 * @return Strong reference created from this weak reference.
	 */
	inline Ref<T> GetRef()const throw(){
		return Ref<T>(*this);
	}



	/**
	 * @brief Destructor.
	 */
	inline ~WeakRef()throw(){
		M_REF_PRINT(<< "WeakRef::~WeakRef(): invoked" << std::endl)
		this->Destroy();
	}



	/**
	 * @brief operator =.
	 * Re-initializes this weak reference object by assignment of RefCounted pointer.
	 * This operator is useful when it is necessary to initialize some weak reference from
	 * right within the constructor of RefCounted-derived object, thus it is possible
	 * to just assign 'this' to the weak reference.
     * @param rc - pointer to RefCounted object.
     * @return reference to this weak reference object.
     */
	inline WeakRef& operator=(T* rc)throw(){
		ASSERT(rc)
		M_REF_PRINT(<< "WeakRef::operator=(T*): invoked" << std::endl)

		this->Destroy();
		this->InitFromRefCounted(rc);
		return *this;
	}



	/**
	 * @brief operator =.
	 * Assign this weak reference from a strong reference.
     * @param r - strong reference to assign from.
     * @return reference to this weak reference object.
     */
	inline WeakRef& operator=(const Ref<T> &r)throw(){
		M_REF_PRINT(<< "WeakRef::operator=(const Ref<T>&): invoked" << std::endl)
		this->Destroy();
		this->InitFromStrongRef(const_cast<Ref<T>&>(r));
		return *this;
	}



	/**
	 * @brief Assignment operator.
	 * Assign this weak reference from another weak reference.
     * @param r - weak reference to assign from.
     * @return reference to this weak reference object.
     */
	inline WeakRef& operator=(const WeakRef& r)throw(){
		M_REF_PRINT(<< "WeakRef::operator=(const WeakRef<TS>&): invoked" << std::endl)
		this->Destroy();
		this->InitFromWeakRef<T>(r);
		return *this;
	}



	/**
	 * @brief Template operator =.
	 * Template operator = for automatic down-casting and const-casting when assigning
	 * this weak reference from another weak reference.
     * @param r - weak reference to assign from.
     * @return reference to this weak reference object.
     */
	template <class TS> inline WeakRef& operator=(const WeakRef<TS>& r)throw(){
		M_REF_PRINT(<< "WeakRef::operator=(const WeakRef<TS>&): invoked" << std::endl)
		this->Destroy();
		this->InitFromWeakRef<TS>(r);
		return *this;
	}



	/**
	 * @brief Reset this reference.
	 * After calling this method the reference becomes invalid, i.e. it
	 * does not refer to any object.
	 */
	inline void Reset()throw(){
		M_REF_PRINT(<< "WeakRef::Reset(): invoked" << std::endl)
		this->Destroy();
		this->counter = 0;
	}



	/**
	 * @brief Check if this weak reference is invalid.
     * @return true if this weak reference is invalid for sure.
	 * @return false otherwise which means that is is unknown if this weak ref is valid or not.
     */
	inline bool IsSurelyInvalid()const throw(){
		return this->counter == 0 || this->counter->numStrongRefs.FetchAndAdd(0) == 0;
	}


private:
	//WeakRef objects can only be created on stack
	//or as a member of other object or array
	static void* operator new(size_t size);
};//~class WeakRef



template <class T> inline Ref<T>::Ref(const WeakRef<T> &r)throw(){
	if(r.counter == 0){
		this->p = 0;
		return;
	}

	//Try incrementing number of strong references.
	//We want to increment only if it is not 0.
	for(ting::s32 guess = 1; ;){//start with 1, not with 0, since we don't want to increment if value is 0.
		ting::s32 oldVal = r.counter->numStrongRefs.CompareAndExchange(guess, guess + 1);
		if(oldVal == 0){
			//there are no strong references, weak ref is invalid
			this->p = 0;
			return;
		}
		if(oldVal == guess){
			//successfully incremented the number of strong references, weak ref is valid
			this->p = r.p;
			return;
		}
		guess = oldVal;
	}
}



}//~namespace
