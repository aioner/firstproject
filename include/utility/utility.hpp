#ifndef UTILITY_HPP
#define UTILITY_HPP

namespace utility
{
	// T must be: no-throw default constructible and no-throw destructible
	template <typename T>
	struct singleton_default
	{
	private:
		struct object_creator
		{
			// This constructor does nothing more than ensure that instance()
			//  is called before main() begins, thus creating the static
			//  T object before multithreading race issues can come up.
			object_creator() { singleton_default<T>::instance(); }
			inline void do_nothing() const { }
		};
		static object_creator create_object;

		singleton_default();

	public:
		typedef T object_type;

		// If, at any point (in user code), singleton_default<T>::instance()
		//  is called, then the following function is instantiated.
		static object_type & instance()
		{
			// This is the object that we return a reference to.
			// It is guaranteed to be created before main() begins because of
			//  the next line.
			static object_type obj;

			// The following line does nothing else than force the instantiation
			//  of singleton_default<T>::create_object, whose constructor is
			//  called before main() begins.
			create_object.do_nothing();

			return obj;
		}
	};
	template <typename T>
	typename singleton_default<T>::object_creator
		singleton_default<T>::create_object;

	template<class T> inline void destruct_ptr(T* pT) {

		if ( pT ) {	

			delete pT;	
			pT = 0;	
		} 
	}

	struct destruct_obj {

		template<class T> void operator() (T* pT) {

			if ( pT )  { 

				delete pT;	
				pT = 0; 
			}
		}
	};

	template<typename T>
	inline T& singleton_object()
	{
		static T obj;
		return obj;
	}
}

#endif