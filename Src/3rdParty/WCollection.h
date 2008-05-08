#pragma once

#include <afx.h>

#define WNEW DEBUG_NEW

#ifndef _ATLTRY
#define _ATLTRY try
#ifdef _AFX
#define _ATLCATCH( e ) catch( CException* e )
#else
#define _ATLCATCH( e ) catch( CAtlException e )
#endif

#define _ATLCATCHALL() catch( ... )

#ifdef _AFX
#define _ATLDELETEEXCEPTION(e) e->Delete();
#else
#define _ATLDELETEEXCEPTION(e) e;
#endif

#define _ATLRETHROW throw
#endif	// _ATLTRY

#ifndef ATLASSERT
#include <assert.h>
#define ATLASSERT assert
#endif ATLASSERT

//#include <afxdisp.h>
// abstract iteration position
/*#ifndef _AFX
struct __POSITION
{
};
#endif
typedef __POSITION* POSITION;
*/
//#include <atlexcept.h>
//#include <atlstr.h>
//using namespace ATL;

__declspec(noreturn) inline void WINAPI WThrow( HRESULT hr )
{
//	TRACE(atlTraceException, 0, _T("WThrow: hr = 0x%x\n"), hr );
#ifdef _AFX
	if( hr == E_OUTOFMEMORY )
	{
		AfxThrowMemoryException();
	}
	else
	{
//		AfxThrowOleException( hr );
	}
#else
	throw ATL::CAtlException( hr );
#endif
};


struct WPlex     // warning variable length structure
{
	WPlex* pNext;
#if (_AFX_PACKING >= 8)
	DWORD dwReserved[1];    // align on 8 byte boundary
#endif
	// BYTE data[maxNum*elementSize];

	void* data() { return this+1; }

	static WPlex* Create(WPlex*& head, size_t nMax, size_t cbElement);
			// like 'calloc' but no zero fill
			// may throw memory exceptions

	void FreeDataChain();       // free this one and links
};

inline WPlex* WPlex::Create( WPlex*& pHead, size_t nMax, size_t nElementSize )
{
	WPlex* pPlex;

	ATLASSERT( nMax > 0 );
	ATLASSERT( nElementSize > 0 );

	pPlex = static_cast< WPlex* >( malloc( sizeof( WPlex )+(nMax*nElementSize) ) );
	if( pPlex == NULL )
	{
		return( NULL );
	}

	pPlex->pNext = pHead;
	pHead = pPlex;

	return( pPlex );
}

inline void WPlex::FreeDataChain()
{
	WPlex* pPlex;

	pPlex = this;
	while( pPlex != NULL )
	{
		WPlex* pNext;

		pNext = pPlex->pNext;
		free( pPlex );
		pPlex = pNext;
	}
}

template< typename T >
class WElementTraitsBase
{
public:
	typedef const T& INARGTYPE;
	typedef T& OUTARGTYPE;

	static void CopyElements( T* pDest, const T* pSrc, size_t nElements )
	{
		for( size_t iElement = 0; iElement < nElements; iElement++ )
		{
			pDest[iElement] = pSrc[iElement];
		}
	}

	static void RelocateElements( T* pDest, T* pSrc, size_t nElements )
	{
		// A simple memmove works for nearly all types.
		// You'll have to override this for types that have pointers to their
		// own members.
		memmove( pDest, pSrc, nElements*sizeof( T ) );
	}
};

template< typename T >
class WDefaultHashTraits
{
public:
	static ULONG Hash( const T& element ) throw()
	{
		return( ULONG( ULONG( element ) ) );
	}
};

template< typename T >
class WDefaultCompareTraits
{
public:
	static bool CompareElements( const T& element1, const T& element2 )
	{
		return( (element1 == element2) != 0 );  // != 0 to handle overloads of operator== that return BOOL instead of bool
	}

	static int CompareElementsOrdered( const T& element1, const T& element2 )
	{
		if( element1 < element2 )
		{
			return( -1 );
		}
		else if( element1 == element2 )
		{
			return( 0 );
		}
		else
		{
			ATLASSERT( element1 > element2 );
			return( 1 );
		}
	}
};

template< typename T >
class WDefaultElementTraits :
	public WElementTraitsBase< T >,
	public WDefaultHashTraits< T >,
	public WDefaultCompareTraits< T >
{
};

template< typename T >
class WElementTraits :
	public WDefaultElementTraits< T >
{
};

template < typename T >
class WDefaultCharTraits
{
};

template <>
class WDefaultCharTraits<char>
{
public:
	static char CharToUpper(char x)
	{
		return (char)toupper(x);
	}

	static char CharToLower(char x)
	{
		return (char)tolower(x);
	}
};

template <>
class WDefaultCharTraits<wchar_t>
{
public:
	static wchar_t CharToUpper(wchar_t x)
	{
		return (wchar_t)towupper(x);
	}

	static wchar_t CharToLower(wchar_t x)
	{
		return (wchar_t)towlower(x);
	}
};


template<>
class WElementTraits< CString > :
	public WDefaultElementTraits< CString >
{
public:
	typedef CString INARGTYPE;
	typedef CString& OUTARGTYPE;

	static ULONG Hash( INARGTYPE str ) throw()
	{
		const char* pch;
		ULONG nHash;

		nHash = 0;
		pch = str;
		while( *pch != 0 )
		{
			nHash = (nHash<<5)+nHash+*pch;
			pch++;
		}

		return( nHash );
	}

	static bool CompareElements( INARGTYPE str1, INARGTYPE str2 ) throw()
	{
		return( str1.CompareNoCase(str2) == 0 );
	}

	static int CompareElementsOrdered( INARGTYPE str1, INARGTYPE str2 ) throw()
	{
		return( str1.CompareNoCase(str2) );
	}
};


template< typename T, class CharTraits = WDefaultCharTraits<T::XCHAR> >
class CStringElementTraitsI :
	public WElementTraitsBase< T >
{
public:
	typedef typename T::PCXSTR INARGTYPE;
	typedef typename T& OUTARGTYPE;

	static ULONG Hash( INARGTYPE str ) throw()
	{
		const T::XCHAR* pch;
		ULONG nHash;

		ATLASSERT( str != NULL );
		nHash = 0;
		pch = str;
		while( *pch != 0 )
		{
			nHash = (nHash<<5)+nHash+CharTraits::CharToUpper(*pch);
			pch++;
		}

		return( nHash );
	}

	static bool CompareElements( INARGTYPE str1, INARGTYPE str2 ) throw()
	{
		return( T::StrTraits::StringCompareIgnore( str1, str2 ) == 0 );
	}

	static int CompareElementsOrdered( INARGTYPE str1, INARGTYPE str2 ) throw()
	{
		return( T::StrTraits::StringCompareIgnore( str1, str2 ) );
	}
};

template< typename T >
class CStringRefElementTraits :
	public WElementTraitsBase< T >
{
public:
	static ULONG Hash( INARGTYPE str ) throw()
	{
		ULONG nHash = 0;
		const T::XCHAR* pch = str;
		while( *pch != 0 )
		{
			nHash = (nHash<<5)+nHash+(*pch);
			pch++;
		}

		return( nHash );
	}

	static bool CompareElements( INARGTYPE element1, INARGTYPE element2 ) throw()
	{
		return( element1 == element2 );
	}

	static int CompareElementsOrdered( INARGTYPE str1, INARGTYPE str2 ) throw()
	{
		return( str1.Compare( str2 ) );
	}
};


template< typename T >
class WPrimitiveElementTraits :
	public WDefaultElementTraits< T >
{
public:
	typedef T INARGTYPE;
	typedef T& OUTARGTYPE;
};

#define _WDECLARE_PRIMITIVE_TRAITS( T ) \
	template<> \
	class WElementTraits< T > : \
		public WPrimitiveElementTraits< T > \
	{ \
	};

_WDECLARE_PRIMITIVE_TRAITS( unsigned char )
_WDECLARE_PRIMITIVE_TRAITS( unsigned short )
_WDECLARE_PRIMITIVE_TRAITS( unsigned int )
_WDECLARE_PRIMITIVE_TRAITS( unsigned long )
_WDECLARE_PRIMITIVE_TRAITS( unsigned __int64 )
_WDECLARE_PRIMITIVE_TRAITS( signed char )
_WDECLARE_PRIMITIVE_TRAITS( char )
_WDECLARE_PRIMITIVE_TRAITS( short )
_WDECLARE_PRIMITIVE_TRAITS( int )
_WDECLARE_PRIMITIVE_TRAITS( long )
_WDECLARE_PRIMITIVE_TRAITS( __int64 )
_WDECLARE_PRIMITIVE_TRAITS( float )
_WDECLARE_PRIMITIVE_TRAITS( double )
_WDECLARE_PRIMITIVE_TRAITS( bool )
//_WDECLARE_PRIMITIVE_TRAITS( wchar_t )

template< typename E, class ETraits = WElementTraits< E > >
class WArray
{
public:
	typedef typename ETraits::INARGTYPE INARGTYPE;
	typedef typename ETraits::OUTARGTYPE OUTARGTYPE;

public:
	WArray() throw();

	size_t GetCount() const throw();
	bool IsEmpty() const throw();
	bool SetCount( size_t nNewSize, int nGrowBy = -1 );

	void FreeExtra() throw();
	void RemoveAll() throw();

	const E& GetAt( size_t iElement ) const throw();
	void SetAt( size_t iElement, INARGTYPE element );
	E& GetAt( size_t iElement ) throw();

	const E* GetData() const throw();
	E* GetData() throw();

	void SetAtGrow( size_t iElement, INARGTYPE element );
	// Add an empty element to the end of the array
	size_t Add();
	// Add an element to the end of the array
	size_t Add( INARGTYPE element );
	size_t Append( const WArray< E, ETraits >& aSrc );
	void Copy( const WArray< E, ETraits >& aSrc );

	const E& operator[]( size_t iElement ) const throw();
	E& operator[]( size_t iElement ) throw();

	void InsertAt( size_t iElement, INARGTYPE element, size_t nCount = 1 );
	void InsertArrayAt( size_t iStart, const WArray< E, ETraits >* paNew );
	void RemoveAt( size_t iElement, size_t nCount = 1 );

#ifdef _DEBUG
	void AssertValid() const;
#endif  // _DEBUG

	bool GrowBuffer( size_t nNewSize );

// Implementation
private:
	E* m_pData;
	size_t m_nSize;
	size_t m_nMaxSize;
	int m_nGrowBy;

private:
	static void CallConstructors( E* pElements, size_t nElements );
	static void CallDestructors( E* pElements, size_t nElements ) throw();

public:
	~WArray() throw();

private:
	// Private to prevent use
	WArray( const WArray& ) throw();
	WArray& operator=( const WArray& ) throw();
};

template< typename E, class ETraits >
inline size_t WArray< E, ETraits >::GetCount() const
{
	return( m_nSize );
}

template< typename E, class ETraits >
inline bool WArray< E, ETraits >::IsEmpty() const
{
	return( m_nSize == 0 );
}

template< typename E, class ETraits >
inline void WArray< E, ETraits >::RemoveAll()
{
	SetCount( 0, -1 );
}

template< typename E, class ETraits >
inline const E& WArray< E, ETraits >::GetAt( size_t iElement ) const
{
	ATLASSERT( iElement < m_nSize );
	return( m_pData[iElement] );
}

template< typename E, class ETraits >
inline void WArray< E, ETraits >::SetAt( size_t iElement, INARGTYPE element )
{
	ATLASSERT( iElement < m_nSize );
	m_pData[iElement] = element;
}

template< typename E, class ETraits >
inline E& WArray< E, ETraits >::GetAt( size_t iElement )
{
	ATLASSERT( iElement < m_nSize );
	return( m_pData[iElement] );
}

template< typename E, class ETraits >
inline const E* WArray< E, ETraits >::GetData() const
{
	return( m_pData );
}

template< typename E, class ETraits >
inline E* WArray< E, ETraits >::GetData()
{
	return( m_pData );
}

template< typename E, class ETraits >
inline size_t WArray< E, ETraits >::Add()
{
	size_t iElement;

	iElement = m_nSize;
	SetCount( m_nSize+1 );

	return( iElement );
}

//#pragma push_macro("new")
//#undef new

template< typename E, class ETraits >
inline size_t WArray< E, ETraits >::Add( INARGTYPE element )
{
	size_t iElement;

	iElement = m_nSize;
	if( iElement >= m_nMaxSize )
	{
		bool bSuccess = GrowBuffer( iElement+1 );
		if( !bSuccess )
		{
			WThrow( E_OUTOFMEMORY );
		}
	}
	::new( m_pData+iElement ) E( element );
	m_nSize++;

	return( iElement );
}

//#pragma pop_macro("new")

template< typename E, class ETraits >
inline const E& WArray< E, ETraits >::operator[]( size_t iElement ) const
{
	ATLASSERT( iElement < m_nSize );
	return( m_pData[iElement] );
}

template< typename E, class ETraits >
inline E& WArray< E, ETraits >::operator[]( size_t iElement )
{
	ATLASSERT( iElement < m_nSize );
	return( m_pData[iElement] );
}

template< typename E, class ETraits >
WArray< E, ETraits >::WArray() :
	m_pData( NULL ),
	m_nSize( 0 ),
	m_nMaxSize( 0 ),
	m_nGrowBy( 0 )
{
}

template< typename E, class ETraits >
WArray< E, ETraits >::~WArray()
{
	if( m_pData != NULL )
	{
		CallDestructors( m_pData, m_nSize );
		free( m_pData );
	}
}

template< typename E, class ETraits >
bool WArray< E, ETraits >::GrowBuffer( size_t nNewSize )
{
	if( nNewSize > m_nMaxSize )
	{
		if( m_pData == NULL )
		{
			size_t nAllocSize =  size_t( m_nGrowBy ) > nNewSize ? size_t( m_nGrowBy ) : nNewSize ;
			m_pData = static_cast< E* >( malloc( nAllocSize*sizeof( E ) ) );
			if( m_pData == NULL )
			{
				return( false );
			}
			m_nMaxSize = nAllocSize;
		}
		else
		{
			// otherwise, grow array
			size_t nGrowBy = m_nGrowBy;
			if( nGrowBy == 0 )
			{
				// heuristically determine growth when nGrowBy == 0
				//  (this avoids heap fragmentation in many situations)
				nGrowBy = m_nSize/8;
				nGrowBy = (nGrowBy < 4) ? 4 : ((nGrowBy > 1024) ? 1024 : nGrowBy);
			}
			size_t nNewMax;
			if( nNewSize < (m_nMaxSize+nGrowBy) )
				nNewMax = m_nMaxSize+nGrowBy;  // granularity
			else
				nNewMax = nNewSize;  // no slush

			ATLASSERT( nNewMax >= m_nMaxSize );  // no wrap around
#ifdef SIZE_T_MAX
			ATLASSERT( nNewMax <= SIZE_T_MAX/sizeof( E ) ); // no overflow
#endif
			E* pNewData = static_cast< E* >( malloc( nNewMax*sizeof( E ) ) );
			if( pNewData == NULL )
			{
				return false;
			}

			// copy new data from old
			ETraits::RelocateElements( pNewData, m_pData, m_nSize );

			// get rid of old stuff (note: no destructors called)
			free( m_pData );
			m_pData = pNewData;
			m_nMaxSize = nNewMax;
		}
	}

	return true;
}	

template< typename E, class ETraits >
bool WArray< E, ETraits >::SetCount( size_t nNewSize, int nGrowBy )
{
	if( nGrowBy != -1 )
	{
		m_nGrowBy = nGrowBy;  // set new size
	}

	if( nNewSize == 0 )
	{
		// shrink to nothing
		if( m_pData != NULL )
		{
			CallDestructors( m_pData, m_nSize );
			free( m_pData );
			m_pData = NULL;
		}
		m_nSize = 0;
		m_nMaxSize = 0;
	}
	else if( nNewSize <= m_nMaxSize )
	{
		// it fits
		if( nNewSize > m_nSize )
		{
			// initialize the new elements
			CallConstructors( m_pData+m_nSize, nNewSize-m_nSize );
		}
		else if( m_nSize > nNewSize )
		{
			// destroy the old elements
			CallDestructors( m_pData+nNewSize, m_nSize-nNewSize );
		}
		m_nSize = nNewSize;
	}
	else
	{
		bool bSuccess;

		bSuccess = GrowBuffer( nNewSize );
		if( !bSuccess )
		{
			return( false );
		}

		// construct new elements
		ATLASSERT( nNewSize > m_nSize );
		CallConstructors( m_pData+m_nSize, nNewSize-m_nSize );

		m_nSize = nNewSize;
	}

	return true;
}

template< typename E, class ETraits >
size_t WArray< E, ETraits >::Append( const WArray< E, ETraits >& aSrc )
{
	ATLASSERT( this != &aSrc );   // cannot append to itself

	size_t nOldSize = m_nSize;
	SetCount( m_nSize+aSrc.m_nSize );
	ETraits::CopyElements( m_pData+nOldSize, aSrc.m_pData, aSrc.m_nSize );

	return( nOldSize );
}

template< typename E, class ETraits >
void WArray< E, ETraits >::Copy( const WArray< E, ETraits >& aSrc )
{
	ATLASSERT( this != &aSrc );   // cannot append to itself

	SetCount( aSrc.m_nSize );
	ETraits::CopyElements( m_pData, aSrc.m_pData, aSrc.m_nSize );
}

template< typename E, class ETraits >
void WArray< E, ETraits >::FreeExtra()
{
	if( m_nSize != m_nMaxSize )
	{
		// shrink to desired size
#ifdef SIZE_T_MAX
		ATLASSERT( m_nSize <= (SIZE_T_MAX/sizeof( E )) ); // no overflow
#endif
		E* pNewData = NULL;
		if( m_nSize != 0 )
		{
			pNewData = (E*)malloc( m_nSize*sizeof( E ) );
			if( pNewData == NULL )
			{
				return;
			}

			// copy new data from old
			ETraits::RelocateElements( pNewData, m_pData, m_nSize );
		}

		// get rid of old stuff (note: no destructors called)
		free( m_pData );
		m_pData = pNewData;
		m_nMaxSize = m_nSize;
	}
}

template< typename E, class ETraits >
void WArray< E, ETraits >::SetAtGrow( size_t iElement, INARGTYPE element )
{
	size_t nOldSize;

	nOldSize = m_nSize;
	if( iElement >= m_nSize )
		SetCount( iElement+1, -1 );
	_ATLTRY
	{
		m_pData[iElement] = element;
	}
	_ATLCATCHALL()
	{
		if( m_nSize != nOldSize )
		{
			SetCount( nOldSize, -1 );
		}
		_ATLRETHROW;
	}
}

template< typename E, class ETraits >
void WArray< E, ETraits >::InsertAt( size_t iElement, INARGTYPE element, size_t nElements /*=1*/)
{
	ATLASSERT( nElements > 0 );     // zero size not allowed

	if( iElement >= m_nSize )
	{
		// adding after the end of the array
		SetCount( iElement+nElements, -1 );   // grow so nIndex is valid
	}
	else
	{
		// inserting in the middle of the array
		size_t nOldSize = m_nSize;
		SetCount( m_nSize+nElements, -1 );  // grow it to new size
		// destroy intial data before copying over it
		CallDestructors( m_pData+nOldSize, nElements );
		// shift old data up to fill gap
		ETraits::RelocateElements( m_pData+(iElement+nElements), m_pData+iElement,
			nOldSize-iElement );

		_ATLTRY
		{
			// re-init slots we copied from
			CallConstructors( m_pData+iElement, nElements );
		}
		_ATLCATCHALL()
		{
			ETraits::RelocateElements( m_pData+iElement, m_pData+(iElement+nElements),
				nOldSize-iElement );
			SetCount( nOldSize, -1 );
			_ATLRETHROW;
		}
	}

	// insert new value in the gap
	ATLASSERT( (iElement+nElements) <= m_nSize );
	for( size_t iNewElement = iElement; iNewElement < (iElement+nElements); iNewElement++ )
	{
		m_pData[iNewElement] = element;
	}
}

template< typename E, class ETraits >
void WArray< E, ETraits >::RemoveAt( size_t iElement, size_t nElements )
{
	ATLASSERT( (iElement+nElements) <= m_nSize );

	// just remove a range
	size_t nMoveCount = m_nSize-(iElement+nElements);
	CallDestructors( m_pData+iElement, nElements );
	if( nMoveCount > 0 )
	{
		ETraits::RelocateElements( m_pData+iElement, m_pData+(iElement+nElements),
			nMoveCount );
	}
	m_nSize -= nElements;
}

template< typename E, class ETraits >
void WArray< E, ETraits >::InsertArrayAt( size_t iStartElement, 
	const WArray< E, ETraits >* paNew )
{
	ATLASSERT( paNew != NULL );

	if( paNew->GetCount() > 0 )
	{
		InsertAt( iStartElement, paNew->GetAt( 0 ), paNew->GetCount() );
		for( size_t iElement = 0; iElement < paNew->GetCount(); iElement++ )
			SetAt( iStartElement+iElement, paNew->GetAt( iElement ) );
	}
}

#ifdef _DEBUG
template< typename E, class ETraits >
void WArray< E, ETraits >::AssertValid() const
{
	if( m_pData == NULL )
	{
		ATLASSERT( m_nSize == 0 );
		ATLASSERT( m_nMaxSize == 0 );
	}
	else
	{
		ATLASSERT( m_nSize <= m_nMaxSize );
	}
}
#endif

//#pragma push_macro("new")
//#undef new

template< typename E, class ETraits >
void WArray< E, ETraits >::CallConstructors( E* pElements, size_t nElements )
{
	size_t iElement;

	_ATLTRY
	{
		for( iElement = 0; iElement < nElements; iElement++ )
		{
			::new( pElements+iElement ) E;
		}
	}
	_ATLCATCHALL()
	{
		while( iElement > 0 )
		{
			iElement--;
			pElements[iElement].~E();
		}

		_ATLRETHROW;
	}
}

//#pragma pop_macro("new")

template< typename E, class ETraits >
void WArray< E, ETraits >::CallDestructors( E* pElements, size_t nElements ) throw()
{
	(void)pElements;

	for( size_t iElement = 0; iElement < nElements; iElement++ )
	{
		pElements[iElement].~E();
	}
}


template< typename E, class ETraits = WElementTraits< E > >
class WList
{
public:
	typedef typename ETraits::INARGTYPE INARGTYPE;

private:
	class CNode :
		public __POSITION
	{
	public:
		CNode()
		{
		}
		CNode( INARGTYPE element ) :
			m_element( element )
		{
		}
		~CNode() throw()
		{
		}

	public:
		CNode* m_pNext;
		CNode* m_pPrev;
		E m_element;

	private:
		CNode( const CNode& ) throw();
	};

public:
	WList( UINT nBlockSize = 10 ) throw();

	size_t GetCount() const throw();
	bool IsEmpty() const throw();

	E& GetHead() throw();
	const E& GetHead() const throw();
	E& GetTail() throw();
	const E& GetTail() const throw();

	E RemoveHead();
	E RemoveTail();
	void RemoveHeadNoReturn() throw();
	void RemoveTailNoReturn() throw();

	POSITION AddHead();
	POSITION AddHead( INARGTYPE element );
	void AddHeadList( const WList< E, ETraits >* plNew );
	POSITION AddTail();
	POSITION AddTail( INARGTYPE element );
	void AddTailList( const WList< E, ETraits >* plNew );

	void RemoveAll() throw();

	POSITION GetHeadPosition() const throw();
	POSITION GetTailPosition() const throw();
	E& GetNext( POSITION& pos ) throw();
	const E& GetNext( POSITION& pos ) const throw();
	E& GetPrev( POSITION& pos ) throw();
	const E& GetPrev( POSITION& pos ) const throw();

	E& GetAt( POSITION pos ) throw();
	const E& GetAt( POSITION pos ) const throw();
	void SetAt( POSITION pos, INARGTYPE element );
	void RemoveAt( POSITION pos ) throw();

	POSITION InsertBefore( POSITION pos, INARGTYPE element );
	POSITION InsertAfter( POSITION pos, INARGTYPE element );

	POSITION Find( INARGTYPE element, POSITION posStartAfter = NULL ) const throw();
	POSITION FindIndex( size_t iElement ) const throw();

	void MoveToHead( POSITION pos ) throw();
	void MoveToTail( POSITION pos ) throw();
	void SwapElements( POSITION pos1, POSITION pos2 ) throw();

#ifdef _DEBUG
	void AssertValid() const;
#endif  // _DEBUG

// Implementation
private:
	CNode* m_pHead;
	CNode* m_pTail;
	size_t m_nElements;
	WPlex* m_pBlocks;
	CNode* m_pFree;
	UINT m_nBlockSize;

private:
	void GetFreeNode();
	CNode* NewNode( CNode* pPrev, CNode* pNext );
	CNode* NewNode( INARGTYPE element, CNode* pPrev, CNode* pNext );
	void FreeNode( CNode* pNode ) throw();

public:
	~WList() throw();

private:
	// Private to prevent use
	WList( const WList& ) throw();
	WList& operator=( const WList& ) throw();
};

template< typename E, class ETraits >
inline size_t WList< E, ETraits >::GetCount() const
{
	return( m_nElements );
}

template< typename E, class ETraits >
inline bool WList< E, ETraits >::IsEmpty() const
{
	return( m_nElements == 0 );
}

template< typename E, class ETraits >
inline E& WList< E, ETraits >::GetHead()
{
	ATLASSERT( m_pHead != NULL );
	return( m_pHead->m_element );
}

template< typename E, class ETraits >
inline const E& WList< E, ETraits >::GetHead() const
{
	ATLASSERT( m_pHead != NULL );
	return( m_pHead->m_element );
}

template< typename E, class ETraits >
inline E& WList< E, ETraits >::GetTail() 
{
	ATLASSERT( m_pTail != NULL );
	return( m_pTail->m_element );
}

template< typename E, class ETraits >
inline const E& WList< E, ETraits >::GetTail() const
{
	ATLASSERT( m_pTail != NULL );
	return( m_pTail->m_element );
}

template< typename E, class ETraits >
inline POSITION WList< E, ETraits >::GetHeadPosition() const
{
	return( POSITION( m_pHead ) );
}

template< typename E, class ETraits >
inline POSITION WList< E, ETraits >::GetTailPosition() const
{
	return( POSITION( m_pTail ) );
}

template< typename E, class ETraits >
inline E& WList< E, ETraits >::GetNext( POSITION& pos )
{
	CNode* pNode;

	ATLASSERT( pos != NULL );
	pNode = (CNode*)pos;
	pos = POSITION( pNode->m_pNext );

	return( pNode->m_element );
}

template< typename E, class ETraits >
inline const E& WList< E, ETraits >::GetNext( POSITION& pos ) const
{
	CNode* pNode;

	ATLASSERT( pos != NULL );
	pNode = (CNode*)pos;
	pos = POSITION( pNode->m_pNext );

	return( pNode->m_element );
}

template< typename E, class ETraits >
inline E& WList< E, ETraits >::GetPrev( POSITION& pos )
{
	CNode* pNode;

	ATLASSERT( pos != NULL );
	pNode = (CNode*)pos;
	pos = POSITION( pNode->m_pPrev );

	return( pNode->m_element );
}

template< typename E, class ETraits >
inline const E& WList< E, ETraits >::GetPrev( POSITION& pos ) const
{
	CNode* pNode;

	ATLASSERT( pos != NULL );
	pNode = (CNode*)pos;
	pos = POSITION( pNode->m_pPrev );

	return( pNode->m_element );
}

template< typename E, class ETraits >
inline E& WList< E, ETraits >::GetAt( POSITION pos )
{
	CNode* pNode;

	ATLASSERT( pos != NULL );
	pNode = (CNode*)pos;

	return( pNode->m_element );
}

template< typename E, class ETraits >
inline const E& WList< E, ETraits >::GetAt( POSITION pos ) const
{
	CNode* pNode;

	ATLASSERT( pos != NULL );
	pNode = (CNode*)pos;

	return( pNode->m_element );
}

template< typename E, class ETraits >
inline void WList< E, ETraits >::SetAt( POSITION pos, INARGTYPE element )
{
	CNode* pNode;

	ATLASSERT( pos != NULL );
	pNode = (CNode*)pos;
	pNode->m_element = element;
}

template< typename E, class ETraits >
WList< E, ETraits >::WList( UINT nBlockSize ) :
	m_nElements( 0 ),
	m_pHead( NULL ),
	m_pTail( NULL ),
	m_nBlockSize( nBlockSize ),
	m_pBlocks( NULL ),
	m_pFree( NULL )
{
	ATLASSERT( nBlockSize > 0 );
}

template< typename E, class ETraits >
void WList< E, ETraits >::RemoveAll()
{
	while( m_nElements > 0 )
	{
		CNode* pKill;

		pKill = m_pHead;
		ATLASSERT( pKill != NULL );
		m_pHead = m_pHead->m_pNext;
		FreeNode( pKill );
	}
	ATLASSERT( m_nElements == 0 );
	m_pHead = NULL;
	m_pTail = NULL;
	m_pFree = NULL;
	m_pBlocks->FreeDataChain();
	m_pBlocks = NULL;
}

template< typename E, class ETraits >
WList< E, ETraits >::~WList()
{
	RemoveAll();
	ATLASSERT( m_nElements == 0 );
}

//#pragma push_macro("new")
//#undef new

template< typename E, class ETraits >
void WList< E, ETraits >::GetFreeNode()
{
	if( m_pFree == NULL )
	{
		WPlex* pPlex;
		CNode* pNode;

		pPlex = WPlex::Create( m_pBlocks, m_nBlockSize, sizeof( CNode ) );
		if( pPlex == NULL )
		{
			WThrow( E_OUTOFMEMORY );
		}
		pNode = (CNode*)pPlex->data();
		pNode += m_nBlockSize-1;
		for( int iBlock = m_nBlockSize-1; iBlock >= 0; iBlock-- )
		{
			pNode->m_pNext = m_pFree;
			m_pFree = pNode;
			pNode--;
		}
	}
	ATLASSERT( m_pFree != NULL );
}

template< typename E, class ETraits >
typename WList< E, ETraits >::CNode* WList< E, ETraits >::NewNode( CNode* pPrev, CNode* pNext )
{
	GetFreeNode();

	CNode* pNewNode = m_pFree;
	CNode* pNextFree = m_pFree->m_pNext;

	::new( pNewNode ) CNode;

	m_pFree = pNextFree;
	pNewNode->m_pPrev = pPrev;
	pNewNode->m_pNext = pNext;
	m_nElements++;
	ATLASSERT( m_nElements > 0 );

	return( pNewNode );
}

template< typename E, class ETraits >
typename WList< E, ETraits >::CNode* WList< E, ETraits >::NewNode( INARGTYPE element, CNode* pPrev, 
	CNode* pNext )
{
	GetFreeNode();

	CNode* pNewNode = m_pFree;
	CNode* pNextFree = m_pFree->m_pNext;

	::new( pNewNode ) CNode( element );

	m_pFree = pNextFree;
	pNewNode->m_pPrev = pPrev;
	pNewNode->m_pNext = pNext;
	m_nElements++;
	ATLASSERT( m_nElements > 0 );

	return( pNewNode );
}

//#pragma pop_macro("new")

template< typename E, class ETraits >
void WList< E, ETraits >::FreeNode( CNode* pNode )
{
	pNode->~CNode();
	pNode->m_pNext = m_pFree;
	m_pFree = pNode;
	ATLASSERT( m_nElements > 0 );
	m_nElements--;
	if( m_nElements == 0 )
	{
		RemoveAll();
	}
}

template< typename E, class ETraits >
POSITION WList< E, ETraits >::AddHead()
{
	CNode* pNode = NewNode( NULL, m_pHead );
	if( m_pHead != NULL )
	{
		m_pHead->m_pPrev = pNode;
	}
	else
	{
		m_pTail = pNode;
	}
	m_pHead = pNode;

	return( POSITION( pNode ) );
}

template< typename E, class ETraits >
POSITION WList< E, ETraits >::AddHead( INARGTYPE element )
{
	CNode* pNode;

	pNode = NewNode( element, NULL, m_pHead );

	if( m_pHead != NULL )
	{
		m_pHead->m_pPrev = pNode;
	}
	else
	{
		m_pTail = pNode;
	}
	m_pHead = pNode;

	return( POSITION( pNode ) );
}

template< typename E, class ETraits >
POSITION WList< E, ETraits >::AddTail()
{
	CNode* pNode = NewNode( m_pTail, NULL );
	if( m_pTail != NULL )
	{
		m_pTail->m_pNext = pNode;
	}
	else
	{
		m_pHead = pNode;
	}
	m_pTail = pNode;

	return( POSITION( pNode ) );
}

template< typename E, class ETraits >
POSITION WList< E, ETraits >::AddTail( INARGTYPE element )
{
	CNode* pNode;

	pNode = NewNode( element, m_pTail, NULL );

	if( m_pTail != NULL )
	{
		m_pTail->m_pNext = pNode;
	}
	else
	{
		m_pHead = pNode;
	}
	m_pTail = pNode;

	return( POSITION( pNode ) );
}

template< typename E, class ETraits >
void WList< E, ETraits >::AddHeadList( const WList< E, ETraits >* plNew )
{
	POSITION pos;

	ATLASSERT( plNew != NULL );

	pos = plNew->GetTailPosition();
	while( pos != NULL )
	{
		INARGTYPE element = plNew->GetPrev( pos );
		AddHead( element );
	}
}

template< typename E, class ETraits >
void WList< E, ETraits >::AddTailList( const WList< E, ETraits >* plNew )
{
	POSITION pos;

	ATLASSERT( plNew != NULL );

	pos = plNew->GetHeadPosition();
	while( pos != NULL )
	{
		INARGTYPE element = plNew->GetNext( pos );
		AddTail( element );
	}
}

template< typename E, class ETraits >
E WList< E, ETraits >::RemoveHead()
{
	CNode* pNode;

	ATLASSERT( m_pHead != NULL );

	pNode = m_pHead;
	E element( pNode->m_element );

	m_pHead = pNode->m_pNext;
	if( m_pHead != NULL )
	{
		m_pHead->m_pPrev = NULL;
	}
	else
	{
		m_pTail = NULL;
	}
	FreeNode( pNode );

	return( element );
}

template< typename E, class ETraits >
void WList< E, ETraits >::RemoveHeadNoReturn() throw()
{
	ATLASSERT( m_pHead != NULL );

	CNode* pNode = m_pHead;
	m_pHead = pNode->m_pNext;
	if( m_pHead != NULL )
	{
		m_pHead->m_pPrev = NULL;
	}
	else
	{
		m_pTail = NULL;
	}
	FreeNode( pNode );
}

template< typename E, class ETraits >
E WList< E, ETraits >::RemoveTail()
{
	CNode* pNode;

	ATLASSERT( m_pTail != NULL );

	pNode = m_pTail;
	E element( pNode->m_element );

	m_pTail = pNode->m_pPrev;
	if( m_pTail != NULL )
	{
		m_pTail->m_pNext = NULL;
	}
	else
	{
		m_pHead = NULL;
	}
	FreeNode( pNode );

	return( element );
}

template< typename E, class ETraits >
void WList< E, ETraits >::RemoveTailNoReturn() throw()
{
	ATLASSERT( m_pTail != NULL );

	CNode* pNode = m_pTail;

	m_pTail = pNode->m_pPrev;
	if( m_pTail != NULL )
	{
		m_pTail->m_pNext = NULL;
	}
	else
	{
		m_pHead = NULL;
	}
	FreeNode( pNode );
}

template< typename E, class ETraits >
POSITION WList< E, ETraits >::InsertBefore( POSITION pos, INARGTYPE element )
{
	if( pos == NULL )
		return AddHead( element ); // insert before nothing -> head of the list

	// Insert it before position
	CNode* pOldNode = (CNode*)pos;
	CNode* pNewNode = NewNode( element, pOldNode->m_pPrev, pOldNode );

	if( pOldNode->m_pPrev != NULL )
	{
		pOldNode->m_pPrev->m_pNext = pNewNode;
	}
	else
	{
		ATLASSERT( pOldNode == m_pHead );
		m_pHead = pNewNode;
	}
	pOldNode->m_pPrev = pNewNode;

	return( POSITION( pNewNode ) );
}

template< typename E, class ETraits >
POSITION WList< E, ETraits >::InsertAfter( POSITION pos, INARGTYPE element )
{
	if( pos == NULL )
		return AddTail( element ); // insert after nothing -> tail of the list

	// Insert it after position
	CNode* pOldNode = (CNode*)pos;
	CNode* pNewNode = NewNode( element, pOldNode, pOldNode->m_pNext );

	if( pOldNode->m_pNext != NULL )
	{
		pOldNode->m_pNext->m_pPrev = pNewNode;
	}
	else
	{
		ATLASSERT( pOldNode == m_pTail );
		m_pTail = pNewNode;
	}
	pOldNode->m_pNext = pNewNode;

	return( POSITION( pNewNode ) );
}

template< typename E, class ETraits >
void WList< E, ETraits >::RemoveAt( POSITION pos )
{
	CNode* pOldNode = (CNode*)pos;

	// remove pOldNode from list
	if( pOldNode == m_pHead )
	{
		m_pHead = pOldNode->m_pNext;
	}
	else
	{
		pOldNode->m_pPrev->m_pNext = pOldNode->m_pNext;
	}
	if( pOldNode == m_pTail )
	{
		m_pTail = pOldNode->m_pPrev;
	}
	else
	{
		pOldNode->m_pNext->m_pPrev = pOldNode->m_pPrev;
	}
	FreeNode( pOldNode );
}

template< typename E, class ETraits >
POSITION WList< E, ETraits >::FindIndex( size_t iElement ) const
{
	if( iElement >= m_nElements )
		return NULL;  // went too far

	CNode* pNode = m_pHead;
	for( size_t iSearch = 0; iSearch < iElement; iSearch++ )
	{
		pNode = pNode->m_pNext;
	}

	return( POSITION( pNode ) );
}

template< typename E, class ETraits >
void WList< E, ETraits >::MoveToHead( POSITION pos ) throw()
{
	ATLASSERT( pos != NULL );

	CNode* pNode = static_cast< CNode* >( pos );
	if( pNode == m_pHead )
	{
		// Already at the head
		return;
	}

	if( pNode->m_pNext == NULL )
	{
		ATLASSERT( pNode == m_pTail );
		m_pTail = pNode->m_pPrev;
	}
	else
	{
		pNode->m_pNext->m_pPrev = pNode->m_pPrev;
	}
	ATLASSERT( pNode->m_pPrev != NULL );  // This node can't be the head, since we already checked that case
	pNode->m_pPrev->m_pNext = pNode->m_pNext;

	m_pHead->m_pPrev = pNode;
	pNode->m_pNext = m_pHead;
	pNode->m_pPrev = NULL;
	m_pHead = pNode;
}

template< typename E, class ETraits >
void WList< E, ETraits >::MoveToTail( POSITION pos ) throw()
{
	ATLASSERT( pos != NULL );

	CNode* pNode = static_cast< CNode* >( pos );
	if( pNode == m_pTail )
	{
		// Already at the tail
		return;
	}

	if( pNode->m_pPrev == NULL )
	{
		ATLASSERT( pNode == m_pHead );
		m_pHead = pNode->m_pNext;
	}
	else
	{
		pNode->m_pPrev->m_pNext = pNode->m_pNext;
	}
	ATLASSERT( pNode->m_pNext != NULL );  // This node can't be the tail, since we already checked that case
	pNode->m_pNext->m_pPrev = pNode->m_pPrev;

	m_pTail->m_pNext = pNode;
	pNode->m_pPrev = m_pTail;
	pNode->m_pNext = NULL;
	m_pTail = pNode;
}

template< typename E, class ETraits >
void WList< E, ETraits >::SwapElements( POSITION pos1, POSITION pos2 ) throw()
{
	ATLASSERT( pos1 != NULL );
	ATLASSERT( pos2 != NULL );

	if( pos1 == pos2 )
	{
		// Nothing to do
		return;
	}

	CNode* pNode1 = static_cast< CNode* >( pos1 );
	CNode* pNode2 = static_cast< CNode* >( pos2 );
	if( pNode2->m_pNext == pNode1 )
	{
		// Swap pNode2 and pNode1 so that the next case works
		CNode* pNodeTemp = pNode1;
		pNode1 = pNode2;
		pNode2 = pNodeTemp;
	}
	if( pNode1->m_pNext == pNode2 )
	{
		// Node1 and Node2 are adjacent
		pNode2->m_pPrev = pNode1->m_pPrev;
		if( pNode1->m_pPrev != NULL )
		{
			pNode1->m_pPrev->m_pNext = pNode2;
		}
		else
		{
			ATLASSERT( m_pHead == pNode1 );
			m_pHead = pNode2;
		}
		pNode1->m_pNext = pNode2->m_pNext;
		if( pNode2->m_pNext != NULL )
		{
			pNode2->m_pNext->m_pPrev = pNode1;
		}
		else
		{
			ATLASSERT( m_pTail == pNode2 );
			m_pTail = pNode1;
		}
		pNode2->m_pNext = pNode1;
		pNode1->m_pPrev = pNode2;
	}
	else
	{
		// The two nodes are not adjacent
		CNode* pNodeTemp;

		pNodeTemp = pNode1->m_pPrev;
		pNode1->m_pPrev = pNode2->m_pPrev;
		pNode2->m_pPrev = pNodeTemp;

		pNodeTemp = pNode1->m_pNext;
		pNode1->m_pNext = pNode2->m_pNext;
		pNode2->m_pNext = pNodeTemp;

		if( pNode1->m_pNext != NULL )
		{
			pNode1->m_pNext->m_pPrev = pNode1;
		}
		else
		{
			ATLASSERT( m_pTail == pNode2 );
			m_pTail = pNode1;
		}
		if( pNode1->m_pPrev != NULL )
		{
			pNode1->m_pPrev->m_pNext = pNode1;
		}
		else
		{
			ATLASSERT( m_pHead == pNode2 );
			m_pHead = pNode1;
		}
		if( pNode2->m_pNext != NULL )
		{
			pNode2->m_pNext->m_pPrev = pNode2;
		}
		else
		{
			ATLASSERT( m_pTail == pNode1 );
			m_pTail = pNode2;
		}
		if( pNode2->m_pPrev != NULL )
		{
			pNode2->m_pPrev->m_pNext = pNode2;
		}
		else
		{
			ATLASSERT( m_pHead == pNode1 );
			m_pHead = pNode2;
		}
	}
}

template< typename E, class ETraits >
POSITION WList< E, ETraits >::Find( INARGTYPE element, POSITION posStartAfter ) const
{
	CNode* pNode = (CNode*)posStartAfter;
	if( pNode == NULL )
	{
		pNode = m_pHead;  // start at head
	}
	else
	{
		pNode = pNode->m_pNext;  // start after the one specified
	}

	for( ; pNode != NULL; pNode = pNode->m_pNext )
	{
		if( ETraits::CompareElements( pNode->m_element, element ) )
			return( POSITION( pNode ) );
	}

	return( NULL );
}

#ifdef _DEBUG
template< typename E, class ETraits >
void WList< E, ETraits >::AssertValid() const
{
	if( IsEmpty() )
	{
		// empty list
		ATLASSERT(m_pHead == NULL);
		ATLASSERT(m_pTail == NULL);
	}
	else
	{
		// non-empty list
	}
}
#endif

template< typename K, typename V, class KTraits = WElementTraits< K >, class VTraits = WElementTraits< V > >
class WMap
{
public:
	typedef typename KTraits::INARGTYPE KINARGTYPE;
	typedef typename KTraits::OUTARGTYPE KOUTARGTYPE;
	typedef typename VTraits::INARGTYPE VINARGTYPE;
	typedef typename VTraits::OUTARGTYPE VOUTARGTYPE;

	class CPair :
		public __POSITION
	{
	protected:
		CPair( KINARGTYPE key ) :
			m_key( key )
		{
		}

	public:
		const K m_key;
		V m_value;
	};

private:
	class CNode :
		public CPair
	{
	public:
		CNode( KINARGTYPE key, UINT nHash ) :
			CPair( key ),
			m_nHash( nHash )
		{
		}

	public:
		UINT GetHash() const throw()
		{
			return( m_nHash );
		}

	public:
		CNode* m_pNext;
		UINT m_nHash;
	};

public:
	WMap( UINT nBins = 17, float fOptimalLoad = 0.75f, 
		float fLoThreshold = 0.25f, float fHiThreshold = 2.25f, UINT nBlockSize = 10 ) throw();

	size_t GetCount() const throw();
	bool IsEmpty() const throw();

	bool Lookup( KINARGTYPE key, VOUTARGTYPE value ) const;
	const CPair* Lookup( KINARGTYPE key ) const throw();
	CPair* Lookup( KINARGTYPE key ) throw();
	V& operator[]( KINARGTYPE key ) throw();

	POSITION SetAt( KINARGTYPE key, VINARGTYPE value );
	void SetValueAt( POSITION pos, VINARGTYPE value );

	bool RemoveKey( KINARGTYPE key ) throw();
	void RemoveAll() throw();
	void RemoveAtPos( POSITION pos ) throw();

	POSITION GetStartPosition() const throw();
	void GetNextAssoc( POSITION& pos, KOUTARGTYPE key, VOUTARGTYPE value ) const;
	const CPair* GetNext( POSITION& pos ) const throw();
	CPair* GetNext( POSITION& pos ) throw();
	const K& GetNextKey( POSITION& pos ) const throw();
	const V& GetNextValue( POSITION& pos ) const throw();
	V& GetNextValue( POSITION& pos ) throw();
	void GetAt( POSITION pos, KOUTARGTYPE key, VOUTARGTYPE value ) const;
	CPair* GetAt( POSITION pos ) throw();
	const CPair* GetAt( POSITION pos ) const throw();
	const K& GetKeyAt( POSITION pos ) const throw();
	const V& GetValueAt( POSITION pos ) const throw();
	V& GetValueAt( POSITION pos ) throw();

	UINT GetHashTableSize() const throw();
	bool InitHashTable( UINT nBins, bool bAllocNow = true );
	void EnableAutoRehash() throw();
	void DisableAutoRehash() throw();
	void Rehash( UINT nBins = 0 );
	void SetOptimalLoad( float fOptimalLoad, float fLoThreshold, float fHiThreshold, 
		bool bRehashNow = false );

#ifdef _DEBUG
	void AssertValid() const;
#endif  // _DEBUG

// Implementation
private:
	CNode** m_ppBins;
	size_t m_nElements;
	UINT m_nBins;
	float m_fOptimalLoad;
	float m_fLoThreshold;
	float m_fHiThreshold;
	size_t m_nHiRehashThreshold;
	size_t m_nLoRehashThreshold;
	ULONG m_nLockCount;
	UINT m_nBlockSize;
	WPlex* m_pBlocks;
	CNode* m_pFree;

private:
	bool IsLocked() const throw();
	UINT PickSize( size_t nElements ) const throw();
	CNode* NewNode( KINARGTYPE key, UINT iBin, UINT nHash );
	void FreeNode( CNode* pNode ) throw();
	void FreePlexes() throw();
	CNode* GetNode( KINARGTYPE key, UINT& iBin, UINT& nHash, CNode*& pPrev ) const throw();
	CNode* CreateNode( KINARGTYPE key, UINT iBin, UINT nHash );
	void RemoveNode( CNode* pNode, CNode* pPrev ) throw();
	CNode* FindNextNode( CNode* pNode ) const throw();
	void UpdateRehashThresholds() throw();

public:
	~WMap() throw();

private:
	// Private to prevent use
	WMap( const WMap& ) throw();
	WMap& operator=( const WMap& ) throw();
};

template< typename K, typename V, class KTraits, class VTraits >
inline size_t WMap< K, V, KTraits, VTraits >::GetCount() const
{
	return( m_nElements );
}

template< typename K, typename V, class KTraits, class VTraits >
inline bool WMap< K, V, KTraits, VTraits >::IsEmpty() const
{
	return( m_nElements == 0 );
}

template< typename K, typename V, class KTraits, class VTraits >
inline V& WMap< K, V, KTraits, VTraits >::operator[]( KINARGTYPE key )
{
	CNode* pNode;
	UINT iBin;
	UINT nHash;
	CNode* pPrev;

	pNode = GetNode( key, iBin, nHash, pPrev );
	if( pNode == NULL )
	{
		pNode = CreateNode( key, iBin, nHash );
	}

	return( pNode->m_value );
}

template< typename K, typename V, class KTraits, class VTraits >
inline UINT WMap< K, V, KTraits, VTraits >::GetHashTableSize() const
{
	return( m_nBins );
}

template< typename K, typename V, class KTraits, class VTraits >
inline void WMap< K, V, KTraits, VTraits >::GetAt( POSITION pos, KOUTARGTYPE key, VOUTARGTYPE value ) const
{
	CNode* pNode;

	ATLASSERT( pos != NULL );

	pNode = static_cast< CNode* >( pos );
	key = pNode->m_key;
	value = pNode->m_value;
}

template< typename K, typename V, class KTraits, class VTraits >
inline typename WMap< K, V, KTraits, VTraits >::CPair* WMap< K, V, KTraits, VTraits >::GetAt( POSITION pos ) throw()
{
	ATLASSERT( pos != NULL );

	return( static_cast< CPair* >( pos ) );
}

template< typename K, typename V, class KTraits, class VTraits >
inline const typename WMap< K, V, KTraits, VTraits >::CPair* WMap< K, V, KTraits, VTraits >::GetAt( POSITION pos ) const throw()
{
	ATLASSERT( pos != NULL );

	return( static_cast< const CPair* >( pos ) );
}

template< typename K, typename V, class KTraits, class VTraits >
inline const K& WMap< K, V, KTraits, VTraits >::GetKeyAt( POSITION pos ) const
{
	CNode* pNode;

	ATLASSERT( pos != NULL );

	pNode = (CNode*)pos;

	return( pNode->m_key );
}

template< typename K, typename V, class KTraits, class VTraits >
inline const V& WMap< K, V, KTraits, VTraits >::GetValueAt( POSITION pos ) const
{
	CNode* pNode;

	ATLASSERT( pos != NULL );

	pNode = (CNode*)pos;

	return( pNode->m_value );
}

template< typename K, typename V, class KTraits, class VTraits >
inline V& WMap< K, V, KTraits, VTraits >::GetValueAt( POSITION pos )
{
	CNode* pNode;

	ATLASSERT( pos != NULL );

	pNode = (CNode*)pos;

	return( pNode->m_value );
}

template< typename K, typename V, class KTraits, class VTraits >
inline void WMap< K, V, KTraits, VTraits >::DisableAutoRehash() throw()
{
	m_nLockCount++;
}

template< typename K, typename V, class KTraits, class VTraits >
inline void WMap< K, V, KTraits, VTraits >::EnableAutoRehash() throw()
{
	ATLASSERT( m_nLockCount > 0 );
	m_nLockCount--;
}

template< typename K, typename V, class KTraits, class VTraits >
inline bool WMap< K, V, KTraits, VTraits >::IsLocked() const
{
	return( m_nLockCount != 0 );
}

template< typename K, typename V, class KTraits, class VTraits >
UINT WMap< K, V, KTraits, VTraits >::PickSize( size_t nElements ) const
{
	// List of primes such that s_anPrimes[i] is the smallest prime greater than 2^(5+i/3)
	static const UINT s_anPrimes[] =
	{
		17, 23, 29, 37, 41, 53, 67, 83, 103, 131, 163, 211, 257, 331, 409, 521, 647, 821, 
		1031, 1291, 1627, 2053, 2591, 3251, 4099, 5167, 6521, 8209, 10331, 
		13007, 16411, 20663, 26017, 32771, 41299, 52021, 65537, 82571, 104033, 
		131101, 165161, 208067, 262147, 330287, 416147, 524309, 660563, 
		832291, 1048583, 1321139, 1664543, 2097169, 2642257, 3329023, 4194319, 
		5284493, 6658049, 8388617, 10568993, 13316089, UINT_MAX
	};

	size_t nBins = (size_t)(nElements/m_fOptimalLoad);
	UINT nBinsEstimate = UINT(  UINT_MAX < nBins ? UINT_MAX : nBins );

	// Find the smallest prime greater than our estimate
	int iPrime = 0;
	while( nBinsEstimate > s_anPrimes[iPrime] )
	{
		iPrime++;
	}

	if( s_anPrimes[iPrime] == UINT_MAX )
	{
		return( nBinsEstimate );
	}
	else
	{
		return( s_anPrimes[iPrime] );
	}
}

template< typename K, typename V, class KTraits, class VTraits >
typename WMap< K, V, KTraits, VTraits >::CNode* WMap< K, V, KTraits, VTraits >::CreateNode( 
	KINARGTYPE key, UINT iBin, UINT nHash )
{
	CNode* pNode;

	if( m_ppBins == NULL )
	{
		bool bSuccess;

		bSuccess = InitHashTable( m_nBins );
		if( !bSuccess )
		{
			WThrow( E_OUTOFMEMORY );
		}
	}

	pNode = NewNode( key, iBin, nHash );

	return( pNode );
}

template< typename K, typename V, class KTraits, class VTraits >
POSITION WMap< K, V, KTraits, VTraits >::GetStartPosition() const
{
	if( IsEmpty() )
	{
		return( NULL );
	}

	for( UINT iBin = 0; iBin < m_nBins; iBin++ )
	{
		if( m_ppBins[iBin] != NULL )
		{
			return( POSITION( m_ppBins[iBin] ) );
		}
	}
	ATLASSERT( false );

	return( NULL );
}

template< typename K, typename V, class KTraits, class VTraits >
POSITION WMap< K, V, KTraits, VTraits >::SetAt( KINARGTYPE key, VINARGTYPE value )
{
	CNode* pNode;
	UINT iBin;
	UINT nHash;
	CNode* pPrev;

	pNode = GetNode( key, iBin, nHash, pPrev );
	if( pNode == NULL )
	{
		pNode = CreateNode( key, iBin, nHash );
		_ATLTRY
		{
			pNode->m_value = value;
		}
		_ATLCATCHALL()
		{
			RemoveAtPos( POSITION( pNode ) );
			_ATLRETHROW;
		}
	}
	else
	{
		pNode->m_value = value;
	}

	return( POSITION( pNode ) );
}

template< typename K, typename V, class KTraits, class VTraits >
void WMap< K, V, KTraits, VTraits >::SetValueAt( POSITION pos, VINARGTYPE value )
{
	ATLASSERT( pos != NULL );

	CNode* pNode = static_cast< CNode* >( pos );
	pNode->m_value = value;
}

template< typename K, typename V, class KTraits, class VTraits >
WMap< K, V, KTraits, VTraits >::WMap( UINT nBins, float fOptimalLoad, 
	float fLoThreshold, float fHiThreshold, UINT nBlockSize ) :
	m_ppBins( NULL ),
	m_nBins( nBins ),
	m_nElements( 0 ),
	m_nLockCount( 0 ),  // Start unlocked
	m_fOptimalLoad( fOptimalLoad ),
	m_fLoThreshold( fLoThreshold ),
	m_fHiThreshold( fHiThreshold ),
	m_nHiRehashThreshold( UINT_MAX ),
	m_nLoRehashThreshold( 0 ),
	m_pBlocks( NULL ),
	m_pFree( NULL ),
	m_nBlockSize( nBlockSize )
{
	ATLASSERT( nBins > 0 );
	ATLASSERT( nBlockSize > 0 );

	SetOptimalLoad( fOptimalLoad, fLoThreshold, fHiThreshold, false );
}

template< typename K, typename V, class KTraits, class VTraits >
void WMap< K, V, KTraits, VTraits >::SetOptimalLoad( float fOptimalLoad, float fLoThreshold,
	float fHiThreshold, bool bRehashNow )
{
	ATLASSERT( fOptimalLoad > 0 );
	ATLASSERT( (fLoThreshold >= 0) && (fLoThreshold < fOptimalLoad) );
	ATLASSERT( fHiThreshold > fOptimalLoad );

	m_fOptimalLoad = fOptimalLoad;
	m_fLoThreshold = fLoThreshold;
	m_fHiThreshold = fHiThreshold;

	UpdateRehashThresholds();

	if( bRehashNow && ((m_nElements > m_nHiRehashThreshold) || 
		(m_nElements < m_nLoRehashThreshold)) )
	{
		Rehash( PickSize( m_nElements ) );
	}
}

template< typename K, typename V, class KTraits, class VTraits >
void WMap< K, V, KTraits, VTraits >::UpdateRehashThresholds() throw()
{
	m_nHiRehashThreshold = size_t( m_fHiThreshold*m_nBins );
	m_nLoRehashThreshold = size_t( m_fLoThreshold*m_nBins );
	if( m_nLoRehashThreshold < 17 )
	{
		m_nLoRehashThreshold = 0;
	}
}

template< typename K, typename V, class KTraits, class VTraits >
bool WMap< K, V, KTraits, VTraits >::InitHashTable( UINT nBins, bool bAllocNow )
{
	ATLASSERT( m_nElements == 0 );
	ATLASSERT( nBins > 0 );

	if( m_ppBins != NULL )
	{
		delete[] m_ppBins;
		m_ppBins = NULL;
	}

	if( bAllocNow )
	{
		ATLTRY( m_ppBins = new CNode*[nBins] );
		if( m_ppBins == NULL )
		{
			return false;
		}
		memset( m_ppBins, 0, sizeof( CNode* )*nBins );
	}
	m_nBins = nBins;

	UpdateRehashThresholds();

	return true;
}

template< typename K, typename V, class KTraits, class VTraits >
void WMap< K, V, KTraits, VTraits >::RemoveAll()
{
	DisableAutoRehash();
	if( m_ppBins != NULL )
	{
		for( UINT iBin = 0; iBin < m_nBins; iBin++ )
		{
			CNode* pNext;

			pNext = m_ppBins[iBin];
			while( pNext != NULL )
			{
				CNode* pKill;

				pKill = pNext;
				pNext = pNext->m_pNext;
				FreeNode( pKill );
			}
		}
	}

	delete[] m_ppBins;
	m_ppBins = NULL;
	m_nElements = 0;

	if( !IsLocked() )
	{
		InitHashTable( PickSize( m_nElements ), false );
	}

	FreePlexes();
	EnableAutoRehash();
}

template< typename K, typename V, class KTraits, class VTraits >
WMap< K, V, KTraits, VTraits >::~WMap()
{
	RemoveAll();
}

//#pragma push_macro("new")
//#undef new

template< typename K, typename V, class KTraits, class VTraits >
typename WMap< K, V, KTraits, VTraits >::CNode* WMap< K, V, KTraits, VTraits >::NewNode(
	KINARGTYPE key, UINT iBin, UINT nHash )
{
	CNode* pNewNode;

	if( m_pFree == NULL )
	{
		WPlex* pPlex;
		CNode* pNode;

		pPlex = WPlex::Create( m_pBlocks, m_nBlockSize, sizeof( CNode ) );
		if( pPlex == NULL )
		{
			WThrow( E_OUTOFMEMORY );
		}
		pNode = (CNode*)pPlex->data();
		pNode += m_nBlockSize-1;
		for( int iBlock = m_nBlockSize-1; iBlock >= 0; iBlock-- )
		{
			pNode->m_pNext = m_pFree;
			m_pFree = pNode;
			pNode--;
		}
	}
	ATLASSERT( m_pFree != NULL );
	pNewNode = m_pFree;
	m_pFree = pNewNode->m_pNext;

	_ATLTRY
	{
		::new( pNewNode ) CNode( key, nHash );
	}
	_ATLCATCHALL()
	{
		pNewNode->m_pNext = m_pFree;
		m_pFree = pNewNode;

		_ATLRETHROW;
	}
	m_nElements++;

	pNewNode->m_pNext = m_ppBins[iBin];
	m_ppBins[iBin] = pNewNode;

	if( (m_nElements > m_nHiRehashThreshold) && !IsLocked() )
	{
		Rehash( PickSize( m_nElements ) );
	}

	return( pNewNode );
}

//#pragma pop_macro("new")

template< typename K, typename V, class KTraits, class VTraits >
void WMap< K, V, KTraits, VTraits >::FreeNode( CNode* pNode )
{
	ATLASSERT( pNode != NULL );

	pNode->~CNode();
	pNode->m_pNext = m_pFree;
	m_pFree = pNode;

	ATLASSERT( m_nElements > 0 );
	m_nElements--;

	if( (m_nElements < m_nLoRehashThreshold) && !IsLocked() )
	{
		Rehash( PickSize( m_nElements ) );
	}

	if( m_nElements == 0 )
	{
		FreePlexes();
	}
}

template< typename K, typename V, class KTraits, class VTraits >
void WMap< K, V, KTraits, VTraits >::FreePlexes() throw()
{
	m_pFree = NULL;
	if( m_pBlocks != NULL )
	{
		m_pBlocks->FreeDataChain();
		m_pBlocks = NULL;
	}
}

template< typename K, typename V, class KTraits, class VTraits >
typename WMap< K, V, KTraits, VTraits >::CNode* WMap< K, V, KTraits, VTraits >::GetNode(
	KINARGTYPE key, UINT& iBin, UINT& nHash, CNode*& pPrev ) const
{
	CNode* pFollow;

	nHash = KTraits::Hash( key );
	iBin = nHash%m_nBins;

	if( m_ppBins == NULL )
	{
		return( NULL );
	}

	pFollow = NULL;
	pPrev = NULL;
	for( CNode* pNode = m_ppBins[iBin]; pNode != NULL; pNode = pNode->m_pNext )
	{
		if( (pNode->GetHash() == nHash) && KTraits::CompareElements( pNode->m_key, key ) )
		{
			pPrev = pFollow;
			return( pNode );
		}
		pFollow = pNode;
	}

	return( NULL );
}

template< typename K, typename V, class KTraits, class VTraits >
bool WMap< K, V, KTraits, VTraits >::Lookup( KINARGTYPE key, VOUTARGTYPE value ) const
{
	UINT iBin;
	UINT nHash;
	CNode* pNode;
	CNode* pPrev;

	pNode = GetNode( key, iBin, nHash, pPrev );
	if( pNode == NULL )
	{
		return( false );
	}

	value = pNode->m_value;

	return( true );
}

template< typename K, typename V, class KTraits, class VTraits >
const typename WMap< K, V, KTraits, VTraits >::CPair* WMap< K, V, KTraits, VTraits >::Lookup( KINARGTYPE key ) const
{
	UINT iBin;
	UINT nHash;
	CNode* pNode;
	CNode* pPrev;

	pNode = GetNode( key, iBin, nHash, pPrev );

	return( pNode );
}

template< typename K, typename V, class KTraits, class VTraits >
typename WMap< K, V, KTraits, VTraits >::CPair* WMap< K, V, KTraits, VTraits >::Lookup( KINARGTYPE key )
{
	UINT iBin;
	UINT nHash;
	CNode* pNode;
	CNode* pPrev;

	pNode = GetNode( key, iBin, nHash, pPrev );

	return( pNode );
}

template< typename K, typename V, class KTraits, class VTraits >
bool WMap< K, V, KTraits, VTraits >::RemoveKey( KINARGTYPE key )
{
	CNode* pNode;
	UINT iBin;
	UINT nHash;
	CNode* pPrev;

	pPrev = NULL;
	pNode = GetNode( key, iBin, nHash, pPrev );
	if( pNode == NULL )
	{
		return( false );
	}

	RemoveNode( pNode, pPrev );

	return( true );
}

template< typename K, typename V, class KTraits, class VTraits >
void WMap< K, V, KTraits, VTraits >::RemoveNode( CNode* pNode, CNode* pPrev )
{
	UINT iBin;

	ATLASSERT( pNode != NULL );

	iBin = pNode->GetHash()%m_nBins;
	if( pPrev == NULL )
	{
		ATLASSERT( m_ppBins[iBin] == pNode );
		m_ppBins[iBin] = pNode->m_pNext;
	}
	else
	{
		ATLASSERT( pPrev->m_pNext == pNode );
		pPrev->m_pNext = pNode->m_pNext;
	}
	FreeNode( pNode );
}

template< typename K, typename V, class KTraits, class VTraits >
void WMap< K, V, KTraits, VTraits >::RemoveAtPos( POSITION pos )
{
	CNode* pNode;
	CNode* pPrev;
	UINT iBin;

	ATLASSERT( pos != NULL );
	pNode = static_cast< CNode* >( pos );
	iBin = pNode->GetHash()%m_nBins;

	ATLASSERT( m_ppBins[iBin] != NULL );
	if( pNode == m_ppBins[iBin] )
	{
		pPrev = NULL;
	}
	else
	{
		pPrev = m_ppBins[iBin];
		while( pPrev->m_pNext != pNode )
		{
			pPrev = pPrev->m_pNext;
			ATLASSERT( pPrev != NULL );
		}
	}
	RemoveNode( pNode, pPrev );
}

template< typename K, typename V, class KTraits, class VTraits >
void WMap< K, V, KTraits, VTraits >::Rehash( UINT nBins )
{
	CNode** ppBins = NULL;

	if( nBins == 0 )
	{
		nBins = PickSize( m_nElements );
	}

	if( nBins == m_nBins )
	{
		return;
	}

//	ATLTRACE(atlTraceMap, 2, _T("Rehash: %u bins\n"), nBins );

	if( m_ppBins == NULL )
	{
		// Just set the new number of bins
		InitHashTable( nBins, false );
		return;
	}

	ATLTRY(ppBins = new CNode*[nBins]);
	if (ppBins == NULL)
	{
		WThrow( E_OUTOFMEMORY );
	}

	memset( ppBins, 0, nBins*sizeof( CNode* ) );

	// Nothing gets copied.  We just rewire the old nodes
	// into the new bins.
	for( UINT iSrcBin = 0; iSrcBin < m_nBins; iSrcBin++ )
	{
		CNode* pNode;

		pNode = m_ppBins[iSrcBin];
		while( pNode != NULL )
		{
			CNode* pNext;
			UINT iDestBin;

			pNext = pNode->m_pNext;  // Save so we don't trash it
			iDestBin = pNode->GetHash()%nBins;
			pNode->m_pNext = ppBins[iDestBin];
			ppBins[iDestBin] = pNode;

			pNode = pNext;
		}
	}

	delete[] m_ppBins;
	m_ppBins = ppBins;
	m_nBins = nBins;

	UpdateRehashThresholds();
}

template< typename K, typename V, class KTraits, class VTraits >
void WMap< K, V, KTraits, VTraits >::GetNextAssoc( POSITION& pos, KOUTARGTYPE key,
	VOUTARGTYPE value ) const
{
	CNode* pNode;
	CNode* pNext;

	ATLASSERT( m_ppBins != NULL );
	ATLASSERT( pos != NULL );

	pNode = (CNode*)pos;
	pNext = FindNextNode( pNode );

	pos = POSITION( pNext );
	key = pNode->m_key;
	value = pNode->m_value;
}

template< typename K, typename V, class KTraits, class VTraits >
const typename WMap< K, V, KTraits, VTraits >::CPair* WMap< K, V, KTraits, VTraits >::GetNext( POSITION& pos ) const
{
	CNode* pNode;
	CNode* pNext;

	ATLASSERT( m_ppBins != NULL );
	ATLASSERT( pos != NULL );

	pNode = (CNode*)pos;
	pNext = FindNextNode( pNode );

	pos = POSITION( pNext );

	return( pNode );
}

template< typename K, typename V, class KTraits, class VTraits >
typename WMap< K, V, KTraits, VTraits >::CPair* WMap< K, V, KTraits, VTraits >::GetNext( 
	POSITION& pos ) throw()
{
	ATLASSERT( m_ppBins != NULL );
	ATLASSERT( pos != NULL );

	CNode* pNode = static_cast< CNode* >( pos );
	CNode* pNext = FindNextNode( pNode );

	pos = POSITION( pNext );

	return( pNode );
}

template< typename K, typename V, class KTraits, class VTraits >
const K& WMap< K, V, KTraits, VTraits >::GetNextKey( POSITION& pos ) const
{
	CNode* pNode;
	CNode* pNext;

	ATLASSERT( m_ppBins != NULL );
	ATLASSERT( pos != NULL );

	pNode = (CNode*)pos;
	pNext = FindNextNode( pNode );

	pos = POSITION( pNext );

	return( pNode->m_key );
}

template< typename K, typename V, class KTraits, class VTraits >
const V& WMap< K, V, KTraits, VTraits >::GetNextValue( POSITION& pos ) const
{
	CNode* pNode;
	CNode* pNext;

	ATLASSERT( m_ppBins != NULL );
	ATLASSERT( pos != NULL );

	pNode = (CNode*)pos;
	pNext = FindNextNode( pNode );

	pos = POSITION( pNext );

	return( pNode->m_value );
}

template< typename K, typename V, class KTraits, class VTraits >
V& WMap< K, V, KTraits, VTraits >::GetNextValue( POSITION& pos )
{
	CNode* pNode;
	CNode* pNext;

	ATLASSERT( m_ppBins != NULL );
	ATLASSERT( pos != NULL );

	pNode = (CNode*)pos;
	pNext = FindNextNode( pNode );

	pos = POSITION( pNext );

	return( pNode->m_value );
}

template< typename K, typename V, class KTraits, class VTraits >
typename WMap< K, V, KTraits, VTraits >::CNode* WMap< K, V, KTraits, VTraits >::FindNextNode( CNode* pNode ) const
{
	CNode* pNext;

	if( pNode->m_pNext != NULL )
	{
		pNext = pNode->m_pNext;
	}
	else
	{
		UINT iBin;

		pNext = NULL;
		iBin = (pNode->GetHash()%m_nBins)+1;
		while( (pNext == NULL) && (iBin < m_nBins) )
		{
			if( m_ppBins[iBin] != NULL )
			{
				pNext = m_ppBins[iBin];
			}

			iBin++;
		}
	}

	return( pNext );
}

#ifdef _DEBUG
template< typename K, typename V, class KTraits, class VTraits >
void WMap< K, V, KTraits, VTraits >::AssertValid() const
{
	ATLASSERT( m_nBins > 0 );
	// non-empty map should have hash table
	ATLASSERT( IsEmpty() || (m_ppBins != NULL) );
}
#endif

//#pragma push_macro("new")
//#undef new

//
// The red-black tree code is based on the the descriptions in
// "Introduction to Algorithms", by Cormen, Leiserson, and Rivest
//
template< typename K, typename V, class KTraits = WElementTraits< K >, class VTraits = WElementTraits< V > >
class WRBTree
{
public:
	typedef typename KTraits::INARGTYPE KINARGTYPE;
	typedef typename KTraits::OUTARGTYPE KOUTARGTYPE;
	typedef typename VTraits::INARGTYPE VINARGTYPE;
	typedef typename VTraits::OUTARGTYPE VOUTARGTYPE;

public:
	class CPair : 
		public __POSITION
	{
	protected:

		CPair( KINARGTYPE key, VINARGTYPE value ) : 
			m_key( key ),
			m_value( value )
		{
		}
		~CPair() throw()
		{
		}

	public:
		const K m_key;
		V m_value;
	};

private:

	class CNode : 
		public CPair
	{
	public:
		enum RB_COLOR
		{
			RB_RED, 
			RB_BLACK
		};

	public:
		RB_COLOR m_eColor;
		CNode* m_pLeft;
		CNode* m_pRight;
		CNode* m_pParent;

		CNode( KINARGTYPE key, VINARGTYPE value ) : 
			CPair( key, value ),
			m_pParent( NULL ),
			m_eColor( RB_BLACK )
		{
		}
		~CNode() throw()
		{
		}
	};

private:
	CNode* m_pRoot;
	size_t m_nCount;
	CNode* m_pFree;
	WPlex* m_pBlocks;
	size_t m_nBlockSize;

	// sentinel node
	CNode *m_pNil;

	// methods
	bool IsNil(CNode *p) const throw();
	void SetNil(CNode **p) throw();

	CNode* NewNode( KINARGTYPE key, VINARGTYPE value );// throw( ... );
	void FreeNode(CNode* pNode) throw();
	void RemovePostOrder(CNode* pNode) throw();
	CNode* LeftRotate(CNode* pNode) throw();
	CNode* RightRotate(CNode* pNode) throw();
	void SwapNode(CNode* pDest, CNode* pSrc) throw();
	CNode* InsertImpl( KINARGTYPE key, VINARGTYPE value );// throw( ... );
	void RBDeleteFixup(CNode* pNode) throw();
	bool RBDelete(CNode* pZ) throw();

#ifdef _DEBUG

	// internal debugging code to verify red-black properties of tree:
	// 1) Every node is either red or black
	// 2) Every leaf (NIL) is black
	// 3) If a node is red, both its children are black
	// 4) Every simple path from a node to a descendant leaf node contains 
	//    the same number of black nodes
private:
	void VerifyIntegrity(const CNode *pNode, int nCurrBlackDepth, int &nBlackDepth) const throw();

public:
	void VerifyIntegrity() const throw();

#endif // _DEBUG

protected:
	CNode* Minimum(CNode* pNode) const throw();
	CNode* Maximum(CNode* pNode) const throw();
	CNode* Predecessor( CNode* pNode ) const throw();
	CNode* Successor(CNode* pNode) const throw();
	CNode* RBInsert( KINARGTYPE key, VINARGTYPE value ); // throw( ... );
	CNode* Find(KINARGTYPE key) const throw();
	CNode* FindPrefix( KINARGTYPE key ) const throw();

protected:
	explicit WRBTree( size_t nBlockSize = 10 ) throw();  // protected to prevent instantiation

public:
	~WRBTree() throw();

	void RemoveAll() throw();
	void RemoveAt(POSITION pos) throw();

	size_t GetCount() const throw();
	bool IsEmpty() const throw();

	POSITION FindFirstKeyAfter( KINARGTYPE key ) const throw();

	POSITION GetHeadPosition() const throw();
	POSITION GetTailPosition() const throw();
	void GetNextAssoc( POSITION& pos, KOUTARGTYPE key, VOUTARGTYPE value ) const;
	const CPair* GetNext(POSITION& pos) const throw();
	CPair* GetNext(POSITION& pos) throw();
	const CPair* GetPrev(POSITION& pos) const throw();
	CPair* GetPrev(POSITION& pos) throw();
	const K& GetNextKey(POSITION& pos) const throw();
	const V& GetNextValue(POSITION& pos) const throw();
	V& GetNextValue(POSITION& pos) throw();

	CPair* GetAt( POSITION pos ) throw();
	const CPair* GetAt( POSITION pos ) const throw();
	void GetAt(POSITION pos, KOUTARGTYPE key, VOUTARGTYPE value) const;
	const K& GetKeyAt(POSITION pos) const throw();
	const V& GetValueAt(POSITION pos) const throw();
	V& GetValueAt(POSITION pos) throw();
	void SetValueAt(POSITION pos, VINARGTYPE value);

private:
	// Private to prevent use
	WRBTree( const WRBTree& ) throw();
	WRBTree& operator=( const WRBTree& ) throw();
};

template< typename K, typename V, class KTraits, class VTraits >
inline bool WRBTree< K, V, KTraits, VTraits >::IsNil(CNode *p) const
{
	return ( p == m_pNil );
}

template< typename K, typename V, class KTraits, class VTraits >
inline void WRBTree< K, V, KTraits, VTraits >::SetNil(CNode **p)
{
	ATLASSERT( p != NULL );

	*p = m_pNil;
}

template< typename K, typename V, class KTraits, class VTraits >
WRBTree< K, V, KTraits, VTraits >::WRBTree( size_t nBlockSize ) throw() :
	m_pRoot( NULL ),
	m_nCount( 0 ),
	m_nBlockSize( nBlockSize ),
	m_pFree( NULL ),
	m_pBlocks( NULL ),
	m_pNil( NULL )
{
	ATLASSERT( nBlockSize > 0 );
}

template< typename K, typename V, class KTraits, class VTraits >
WRBTree< K, V, KTraits, VTraits >::~WRBTree() throw()
{
	RemoveAll();
	if (m_pNil != NULL)
	{
		free(m_pNil);
	}
}

template< typename K, typename V, class KTraits, class VTraits >
void WRBTree< K, V, KTraits, VTraits >::RemoveAll() throw()
{
	if (!IsNil(m_pRoot))
		RemovePostOrder(m_pRoot);
	m_nCount = 0;
	m_pBlocks->FreeDataChain();
	m_pBlocks = NULL;
	m_pFree = NULL;
	m_pRoot = m_pNil;
}

template< typename K, typename V, class KTraits, class VTraits >
size_t WRBTree< K, V, KTraits, VTraits >::GetCount() const throw()
{
	return m_nCount;
}

template< typename K, typename V, class KTraits, class VTraits >
bool WRBTree< K, V, KTraits, VTraits >::IsEmpty() const throw()
{
	return( m_nCount == 0 );
}

template< typename K, typename V, class KTraits, class VTraits >
POSITION WRBTree< K, V, KTraits, VTraits >::FindFirstKeyAfter( KINARGTYPE key ) const throw()
{
	return( FindPrefix( key ) );
}

template< typename K, typename V, class KTraits, class VTraits >
void WRBTree< K, V, KTraits, VTraits >::RemoveAt(POSITION pos) throw()
{
	ATLASSERT(pos != NULL);
	RBDelete(static_cast<CNode*>(pos));
}

template< typename K, typename V, class KTraits, class VTraits >
POSITION WRBTree< K, V, KTraits, VTraits >::GetHeadPosition() const throw()
{
	return( Minimum( m_pRoot ) );
}

template< typename K, typename V, class KTraits, class VTraits >
POSITION WRBTree< K, V, KTraits, VTraits >::GetTailPosition() const throw()
{
	return( Maximum( m_pRoot ) );
}

template< typename K, typename V, class KTraits, class VTraits >
void WRBTree< K, V, KTraits, VTraits >::GetNextAssoc( POSITION& pos, KOUTARGTYPE key, VOUTARGTYPE value ) const
{
	ATLASSERT(pos != NULL);
	CNode* pNode = static_cast< CNode* >(pos);

	key = pNode->m_key;
	value = pNode->m_value;

	pos = Successor(pNode);
}

template< typename K, typename V, class KTraits, class VTraits >
const typename WRBTree< K, V, KTraits, VTraits >::CPair* WRBTree< K, V, KTraits, VTraits >::GetNext(POSITION& pos) const throw()
{
	ATLASSERT(pos != NULL);
	CNode* pNode = static_cast< CNode* >(pos);
	pos = Successor(pNode);
	return pNode;
}

template< typename K, typename V, class KTraits, class VTraits >
typename WRBTree< K, V, KTraits, VTraits >::CPair* WRBTree< K, V, KTraits, VTraits >::GetNext(POSITION& pos) throw()
{
	ATLASSERT(pos != NULL);
	CNode* pNode = static_cast< CNode* >(pos);
	pos = Successor(pNode);
	return pNode;
}

template< typename K, typename V, class KTraits, class VTraits >
const typename WRBTree< K, V, KTraits, VTraits >::CPair* WRBTree< K, V, KTraits, VTraits >::GetPrev(POSITION& pos) const throw()
{
	ATLASSERT(pos != NULL);
	CNode* pNode = static_cast< CNode* >(pos);
	pos = Predecessor(pNode);

	return pNode;
}

template< typename K, typename V, class KTraits, class VTraits >
typename WRBTree< K, V, KTraits, VTraits >::CPair* WRBTree< K, V, KTraits, VTraits >::GetPrev(POSITION& pos) throw()
{
	ATLASSERT(pos != NULL);
	CNode* pNode = static_cast< CNode* >(pos);
	pos = Predecessor(pNode);

	return pNode;
}

template< typename K, typename V, class KTraits, class VTraits >
const K& WRBTree< K, V, KTraits, VTraits >::GetNextKey(POSITION& pos) const throw()
{
	ATLASSERT(pos != NULL);
	CNode* pNode = static_cast<CNode*>(pos);
	pos = Successor(pNode);

	return pNode->m_key;
}

template< typename K, typename V, class KTraits, class VTraits >
const V& WRBTree< K, V, KTraits, VTraits >::GetNextValue(POSITION& pos) const throw()
{
	ATLASSERT(pos != NULL);
	CNode* pNode = static_cast<CNode*>(pos);
	pos = Successor(pNode);

	return pNode->m_value;
}

template< typename K, typename V, class KTraits, class VTraits >
V& WRBTree< K, V, KTraits, VTraits >::GetNextValue(POSITION& pos) throw()
{
	ATLASSERT(pos != NULL);
	CNode* pNode = static_cast<CNode*>(pos);
	pos = Successor(pNode);

	return pNode->m_value;
}

template< typename K, typename V, class KTraits, class VTraits >
typename WRBTree< K, V, KTraits, VTraits >::CPair* WRBTree< K, V, KTraits, VTraits >::GetAt( POSITION pos ) throw()
{
	ATLASSERT( pos != NULL );

	return( static_cast< CPair* >( pos ) );
}

template< typename K, typename V, class KTraits, class VTraits >
const typename WRBTree< K, V, KTraits, VTraits >::CPair* WRBTree< K, V, KTraits, VTraits >::GetAt( POSITION pos ) const throw()
{
	ATLASSERT( pos != NULL );

	return( static_cast< const CPair* >( pos ) );
}

template< typename K, typename V, class KTraits, class VTraits >
void WRBTree< K, V, KTraits, VTraits >::GetAt(POSITION pos, KOUTARGTYPE key, VOUTARGTYPE value) const
{
	ATLASSERT(pos != NULL);
	key = static_cast<CNode*>(pos)->m_key;
	value = static_cast<CNode*>(pos)->m_value;
}

template< typename K, typename V, class KTraits, class VTraits >
const K& WRBTree< K, V, KTraits, VTraits >::GetKeyAt(POSITION pos) const throw()
{
	ATLASSERT(pos != NULL);
	return static_cast<CNode*>(pos)->m_key;
}

template< typename K, typename V, class KTraits, class VTraits >
const V& WRBTree< K, V, KTraits, VTraits >::GetValueAt(POSITION pos) const throw()
{
	ATLASSERT(pos != NULL);
	return static_cast<CNode*>(pos)->m_value;
}

template< typename K, typename V, class KTraits, class VTraits >
V& WRBTree< K, V, KTraits, VTraits >::GetValueAt(POSITION pos) throw()
{
	ATLASSERT(pos != NULL);
	return static_cast<CNode*>(pos)->m_value;
}

template< typename K, typename V, class KTraits, class VTraits >
void WRBTree< K, V, KTraits, VTraits >::SetValueAt(POSITION pos, VINARGTYPE value)
{
	ATLASSERT(pos != NULL);
	static_cast<CNode*>(pos)->m_value = value;
}

template< typename K, typename V, class KTraits, class VTraits >
typename WRBTree< K, V, KTraits, VTraits >::CNode* WRBTree< K, V, KTraits, VTraits >::NewNode( KINARGTYPE key, VINARGTYPE value ) // throw( ... )
{
	if( m_pFree == NULL )
	{
		if (m_pNil == NULL)
		{
			m_pNil = reinterpret_cast<CNode *>(malloc(sizeof( CNode )));
			if (m_pNil == NULL)
			{
				WThrow( E_OUTOFMEMORY );
			}
			memset(m_pNil, 0x00, sizeof(CNode));
			m_pNil->m_eColor = CNode::RB_BLACK;
			m_pNil->m_pParent = m_pNil->m_pLeft = m_pNil->m_pRight = m_pNil;
			m_pRoot = m_pNil;
		}

		WPlex* pPlex = WPlex::Create( m_pBlocks, m_nBlockSize, sizeof( CNode ) );
		if( pPlex == NULL )
		{
			WThrow( E_OUTOFMEMORY );
		}
		CNode* pNode = static_cast< CNode* >( pPlex->data() );
		pNode += m_nBlockSize-1;
		for( INT_PTR iBlock = m_nBlockSize-1; iBlock >= 0; iBlock-- )
		{
			pNode->m_pLeft = m_pFree;
			m_pFree = pNode;
			pNode--;
		}
	}
	ATLASSERT( m_pFree != NULL );

	CNode* pNewNode = m_pFree;
	::new( pNewNode ) CNode( key, value );

	m_pFree = m_pFree->m_pLeft;
	pNewNode->m_eColor = CNode::RB_RED;
	SetNil(&pNewNode->m_pLeft);
	SetNil(&pNewNode->m_pRight);
	SetNil(&pNewNode->m_pParent);

	m_nCount++;
	ATLASSERT( m_nCount > 0 );

	return( pNewNode );
}

template< typename K, typename V, class KTraits, class VTraits >
void WRBTree< K, V, KTraits, VTraits >::FreeNode(CNode* pNode) throw()
{
	ATLASSERT(pNode != NULL);
	pNode->~CNode();
	pNode->m_pLeft = m_pFree;
	m_pFree = pNode;
	ATLASSERT( m_nCount > 0 );
	m_nCount--;
}

template< typename K, typename V, class KTraits, class VTraits >
void WRBTree< K, V, KTraits, VTraits >::RemovePostOrder(CNode* pNode) throw()
{
	if (IsNil(pNode))
		return;
	RemovePostOrder(pNode->m_pLeft);
	RemovePostOrder(pNode->m_pRight);
	FreeNode( pNode );
}

template< typename K, typename V, class KTraits, class VTraits >
typename WRBTree< K, V, KTraits, VTraits >::CNode* WRBTree< K, V, KTraits, VTraits >::LeftRotate(CNode* pNode) throw()
{
	ATLASSERT(pNode != NULL);
	CNode* pRight = pNode->m_pRight;
	pNode->m_pRight = pRight->m_pLeft;
	if (!IsNil(pRight->m_pLeft))
		pRight->m_pLeft->m_pParent = pNode;

	pRight->m_pParent = pNode->m_pParent;
	if (IsNil(pNode->m_pParent))
		m_pRoot = pRight;
	else if (pNode == pNode->m_pParent->m_pLeft)
		pNode->m_pParent->m_pLeft = pRight;
	else 
		pNode->m_pParent->m_pRight = pRight;

	pRight->m_pLeft = pNode;
	pNode->m_pParent = pRight;
	return pNode;

}

template< typename K, typename V, class KTraits, class VTraits >
typename WRBTree< K, V, KTraits, VTraits >::CNode* WRBTree< K, V, KTraits, VTraits >::RightRotate(CNode* pNode) throw()
{
	ATLASSERT(pNode != NULL);
	CNode* pLeft = pNode->m_pLeft;
	pNode->m_pLeft = pLeft->m_pRight;
	if (!IsNil(pLeft->m_pRight))
		pLeft->m_pRight->m_pParent = pNode;

	pLeft->m_pParent = pNode->m_pParent;
	if (IsNil(pNode->m_pParent))
		m_pRoot = pLeft;
	else if (pNode == pNode->m_pParent->m_pRight)
		pNode->m_pParent->m_pRight = pLeft;
	else
		pNode->m_pParent->m_pLeft = pLeft;

	pLeft->m_pRight = pNode;
	pNode->m_pParent = pLeft;
	return pNode;
}

template< typename K, typename V, class KTraits, class VTraits >
typename WRBTree< K, V, KTraits, VTraits >::CNode* WRBTree< K, V, KTraits, VTraits >::Find(KINARGTYPE key) const throw()
{
	CNode* pKey = NULL;
	CNode* pNode = m_pRoot;
	while( !IsNil(pNode) && (pKey == NULL) )
	{
		int nCompare = KTraits::CompareElementsOrdered( key, pNode->m_key );
		if( nCompare == 0 )
		{
			pKey = pNode;
		}
		else
		{
			if( nCompare < 0 )
			{
				pNode = pNode->m_pLeft;
			}
			else
			{
				pNode = pNode->m_pRight;
			}
		}
	}

	if( pKey == NULL )
	{
		return( NULL );
	}

#pragma warning(push)
#pragma warning(disable:4127)

	while( true )
	{
		CNode* pPrev = Predecessor( pKey );
		if( (pPrev != NULL) && KTraits::CompareElements( key, pPrev->m_key ) )
		{
			pKey = pPrev;
		}
		else
		{
			return( pKey );
		}
	}

#pragma warning(pop)
}

template< typename K, typename V, class KTraits, class VTraits >
typename WRBTree< K, V, KTraits, VTraits >::CNode* WRBTree< K, V, KTraits, VTraits >::FindPrefix( KINARGTYPE key ) const throw()
{
	// First, attempt to find a node that matches the key exactly
	CNode* pParent = NULL;
	CNode* pKey = NULL;
	CNode* pNode = m_pRoot;
	while( !IsNil(pNode) && (pKey == NULL) )
	{
		pParent = pNode;
		int nCompare = KTraits::CompareElementsOrdered( key, pNode->m_key );
		if( nCompare == 0 )
		{
			pKey = pNode;
		}
		else if( nCompare < 0 )
		{
			pNode = pNode->m_pLeft;
		}
		else
		{
			pNode = pNode->m_pRight;
		}
	}

	if( pKey != NULL )
	{
		// We found a node with the exact key, so find the first node in 
		// the tree with that key by walking backwards until we find a node
		// that doesn't match the key
		while( true )
		{
			CNode* pPrev = Predecessor( pKey );
			if( (pPrev != NULL) && KTraits::CompareElements( key, pPrev->m_key ) )
			{
				pKey = pPrev;
			}
			else
			{
				return( pKey );
			}
		}
	}
	else if (pParent != NULL)
	{
		// No node matched the key exactly, so pick the first node with 
		// a key greater than the given key
		int nCompare = KTraits::CompareElementsOrdered( key, pParent->m_key );
		if( nCompare < 0 )
		{
			pKey = pParent;
		}
		else
		{
			ATLASSERT( nCompare > 0 );
			pKey = Successor( pParent );
		}
	}

	return( pKey );
}

template< typename K, typename V, class KTraits, class VTraits >
void WRBTree< K, V, KTraits, VTraits >::SwapNode(CNode* pDest, CNode* pSrc) throw()
{
	ATLASSERT(pDest != NULL);
	ATLASSERT(pSrc != NULL);

	pDest->m_pParent = pSrc->m_pParent;
	if (pSrc->m_pParent->m_pLeft == pSrc)
		pSrc->m_pParent->m_pLeft = pDest;
	else
		pSrc->m_pParent->m_pRight = pDest;

	pDest->m_pRight = pSrc->m_pRight;
	pDest->m_pLeft = pSrc->m_pLeft;
	pDest->m_eColor = pSrc->m_eColor;
	pDest->m_pRight->m_pParent = pDest;
	pDest->m_pLeft->m_pParent = pDest;
	if (m_pRoot == pSrc)
	{
		m_pRoot = pDest;
	}
}

template< typename K, typename V, class KTraits, class VTraits >
typename WRBTree< K, V, KTraits, VTraits >::CNode* WRBTree< K, V, KTraits, VTraits >::InsertImpl( KINARGTYPE key, VINARGTYPE value ) //throw( ... )
{
	CNode* pNew = NewNode( key, value );

	CNode* pY = NULL;
	CNode* pX = m_pRoot;

	while (!IsNil(pX))
	{
		pY = pX;
		if( KTraits::CompareElementsOrdered( key, pX->m_key ) <= 0 )
			pX = pX->m_pLeft;
		else
			pX = pX->m_pRight;
	}

	pNew->m_pParent = pY;
	if (pY == NULL)
	{
		m_pRoot = pNew;
	}
	else if( KTraits::CompareElementsOrdered( key, pY->m_key ) <= 0 )
		pY->m_pLeft = pNew;
	else
		pY->m_pRight = pNew;

	return pNew;
}

template< typename K, typename V, class KTraits, class VTraits >
void WRBTree< K, V, KTraits, VTraits >::RBDeleteFixup(CNode* pNode) throw()
{
	CNode* pX = pNode;
	CNode* pW = NULL;

	while (pX != m_pRoot && pX->m_eColor == CNode::RB_BLACK)
	{
		if (pX == pX->m_pParent->m_pLeft)
		{
			pW = pX->m_pParent->m_pRight;
			if (pW->m_eColor == CNode::RB_RED)
			{
				pW->m_eColor = CNode::RB_BLACK;
				pW->m_pParent->m_eColor = CNode::RB_RED;
				LeftRotate(pX->m_pParent);
				pW = pX->m_pParent->m_pRight;
			}
			if (pW->m_pLeft->m_eColor == CNode::RB_BLACK && pW->m_pRight->m_eColor == CNode::RB_BLACK)
			{
				pW->m_eColor = CNode::RB_RED;
				pX = pX->m_pParent;
			}
			else
			{
				if (pW->m_pRight->m_eColor == CNode::RB_BLACK)
				{
					pW->m_pLeft->m_eColor = CNode::RB_BLACK;
					pW->m_eColor = CNode::RB_RED;
					RightRotate(pW);
					pW = pX->m_pParent->m_pRight;
				}
				pW->m_eColor = pX->m_pParent->m_eColor;
				pX->m_pParent->m_eColor = CNode::RB_BLACK;
				pW->m_pRight->m_eColor = CNode::RB_BLACK;
				LeftRotate(pX->m_pParent);
				pX = m_pRoot;
			}
		}
		else
		{
			pW = pX->m_pParent->m_pLeft;
			if (pW->m_eColor == CNode::RB_RED)
			{
				pW->m_eColor = CNode::RB_BLACK;
				pW->m_pParent->m_eColor = CNode::RB_RED;
				RightRotate(pX->m_pParent);
				pW = pX->m_pParent->m_pLeft;
			}
			if (pW->m_pRight->m_eColor == CNode::RB_BLACK && pW->m_pLeft->m_eColor == CNode::RB_BLACK)
			{
				pW->m_eColor = CNode::RB_RED;
				pX = pX->m_pParent;
			}
			else
			{
				if (pW->m_pLeft->m_eColor == CNode::RB_BLACK)
				{
					pW->m_pRight->m_eColor = CNode::RB_BLACK;
					pW->m_eColor = CNode::RB_RED;
					LeftRotate(pW);
					pW = pX->m_pParent->m_pLeft;
				}
				pW->m_eColor = pX->m_pParent->m_eColor;
				pX->m_pParent->m_eColor = CNode::RB_BLACK;
				pW->m_pLeft->m_eColor = CNode::RB_BLACK;
				RightRotate(pX->m_pParent);
				pX = m_pRoot;
			}
		}
	}

	pX->m_eColor = CNode::RB_BLACK;
}


template< typename K, typename V, class KTraits, class VTraits >
bool WRBTree< K, V, KTraits, VTraits >::RBDelete(CNode* pZ) throw()
{
	if (pZ == NULL)
		return false;

	CNode* pY = NULL;
	CNode* pX = NULL;
	if (IsNil(pZ->m_pLeft) || IsNil(pZ->m_pRight))
		pY = pZ;
	else
		pY = Successor(pZ);

	if (!IsNil(pY->m_pLeft))
		pX = pY->m_pLeft;
	else
		pX = pY->m_pRight;

	pX->m_pParent = pY->m_pParent;

	if (IsNil(pY->m_pParent))
		m_pRoot = pX;
	else if (pY == pY->m_pParent->m_pLeft)
		pY->m_pParent->m_pLeft = pX;
	else
		pY->m_pParent->m_pRight = pX;

	if (pY->m_eColor == CNode::RB_BLACK)
		RBDeleteFixup(pX);

	if (pY != pZ)
		SwapNode(pY, pZ);

	if (m_pRoot != NULL)
		SetNil(&m_pRoot->m_pParent);

	FreeNode( pZ );

	return true;
}

template< typename K, typename V, class KTraits, class VTraits >
typename WRBTree< K, V, KTraits, VTraits >::CNode* WRBTree< K, V, KTraits, VTraits >::Minimum(CNode* pNode) const throw()
{
	if (pNode == NULL || IsNil(pNode))
	{
		return NULL;
	}

	CNode* pMin = pNode;
	while (!IsNil(pMin->m_pLeft))
	{
		pMin = pMin->m_pLeft;
	}

	return pMin;
}

template< typename K, typename V, class KTraits, class VTraits >
typename WRBTree< K, V, KTraits, VTraits >::CNode* WRBTree< K, V, KTraits, VTraits >::Maximum(CNode* pNode) const throw()
{
	if (pNode == NULL || IsNil(pNode))
	{
		return NULL;
	}

	CNode* pMax = pNode;
	while (!IsNil(pMax->m_pRight))
	{
		pMax = pMax->m_pRight;
	}

	return pMax;
}

template< typename K, typename V, class KTraits, class VTraits >
typename WRBTree< K, V, KTraits, VTraits >::CNode* WRBTree< K, V, KTraits, VTraits >::Predecessor( CNode* pNode ) const throw()
{
	if( pNode == NULL )
	{
		return( NULL );
	}
	if( !IsNil(pNode->m_pLeft) )
	{
		return( Maximum( pNode->m_pLeft ) );
	}

	CNode* pParent = pNode->m_pParent;
	CNode* pLeft = pNode;
	while( !IsNil(pParent) && (pLeft == pParent->m_pLeft) )
	{
		pLeft = pParent;
		pParent = pParent->m_pParent;
	}

	if (IsNil(pParent))
	{
		pParent = NULL;
	}
	return( pParent );
}

template< typename K, typename V, class KTraits, class VTraits >
typename WRBTree< K, V, KTraits, VTraits >::CNode* WRBTree< K, V, KTraits, VTraits >::Successor(CNode* pNode) const throw()
{
	if ( pNode == NULL )
	{
		return NULL;
	}
	if ( !IsNil(pNode->m_pRight) )
	{
		return Minimum(pNode->m_pRight);
	}

	CNode* pParent = pNode->m_pParent;
	CNode* pRight = pNode;
	while ( !IsNil(pParent) && (pRight == pParent->m_pRight) )
	{
		pRight = pParent;
		pParent = pParent->m_pParent;
	}

	if (IsNil(pParent))
	{
		pParent = NULL;
	}
	return pParent;
}

template< typename K, typename V, class KTraits, class VTraits >
typename WRBTree< K, V, KTraits, VTraits >::CNode* WRBTree< K, V, KTraits, VTraits >::RBInsert( KINARGTYPE key, VINARGTYPE value ) //throw( ... )
{
	CNode* pNewNode = InsertImpl( key, value );

	CNode* pX = pNewNode;
	pX->m_eColor = CNode::RB_RED;
	CNode* pY = NULL;
	while (pX != m_pRoot && pX->m_pParent->m_eColor == CNode::RB_RED)
	{
		if (pX->m_pParent == pX->m_pParent->m_pParent->m_pLeft)
		{
			pY = pX->m_pParent->m_pParent->m_pRight;
			if (pY != NULL && pY->m_eColor == CNode::RB_RED)
			{
				pX->m_pParent->m_eColor = CNode::RB_BLACK;
				pY->m_eColor = CNode::RB_BLACK;
				pX->m_pParent->m_pParent->m_eColor = CNode::RB_RED;
				pX = pX->m_pParent->m_pParent;
			}
			else
			{
				if (pX == pX->m_pParent->m_pRight)
				{
					pX = pX->m_pParent;
					LeftRotate(pX);
				}
				pX->m_pParent->m_eColor = CNode::RB_BLACK;
				pX->m_pParent->m_pParent->m_eColor = CNode::RB_RED;
				RightRotate(pX->m_pParent->m_pParent);
			}
		}
		else
		{
			pY = pX->m_pParent->m_pParent->m_pLeft;
			if (pY != NULL && pY->m_eColor == CNode::RB_RED)
			{
				pX->m_pParent->m_eColor = CNode::RB_BLACK;
				pY->m_eColor = CNode::RB_BLACK;
				pX->m_pParent->m_pParent->m_eColor = CNode::RB_RED;
				pX = pX->m_pParent->m_pParent;
			}
			else
			{
				if (pX == pX->m_pParent->m_pLeft)
				{
					pX = pX->m_pParent;
					RightRotate(pX);
				}
				pX->m_pParent->m_eColor = CNode::RB_BLACK;
				pX->m_pParent->m_pParent->m_eColor = CNode::RB_RED;
				LeftRotate(pX->m_pParent->m_pParent);
			}
		}
	}

	m_pRoot->m_eColor = CNode::RB_BLACK;
	SetNil(&m_pRoot->m_pParent);

	return( pNewNode );
}

#ifdef _DEBUG

template< typename K, typename V, class KTraits, class VTraits >
void WRBTree< K, V, KTraits, VTraits >::VerifyIntegrity(const CNode *pNode, int nCurrBlackDepth, int &nBlackDepth) const
{
	bool bCheckForBlack = false;
	bool bLeaf = true;

	if (pNode->m_eColor == CNode::RB_RED) 
		bCheckForBlack = true;
	else
		nCurrBlackDepth++;

	ATLASSERT(pNode->m_pLeft != NULL);
	if (!IsNil(pNode->m_pLeft))
	{
		bLeaf = false;
		if (bCheckForBlack)
		{
			ATLASSERT(pNode->m_pLeft->m_eColor == CNode::RB_BLACK);
		}

		VerifyIntegrity(pNode->m_pLeft, nCurrBlackDepth, nBlackDepth);
	}

	ATLASSERT(pNode->m_pRight != NULL);
	if (!IsNil(pNode->m_pRight))
	{
		bLeaf = false;
		if (bCheckForBlack)
		{
			ATLASSERT(pNode->m_pRight->m_eColor == CNode::RB_BLACK);
		}

		VerifyIntegrity(pNode->m_pRight, nCurrBlackDepth, nBlackDepth);
	}

	ATLASSERT( pNode->m_pParent != NULL );
	ATLASSERT( ( IsNil(pNode->m_pParent) ) ||
			( pNode->m_pParent->m_pLeft == pNode ) ||
			( pNode->m_pParent->m_pRight == pNode ) );

	if (bLeaf) 
	{
		if (nBlackDepth == 0)
		{
			nBlackDepth = nCurrBlackDepth;
		}
		else
		{
			ATLASSERT(nBlackDepth == nCurrBlackDepth);
		}
	}
}

template< typename K, typename V, class KTraits, class VTraits >
void WRBTree< K, V, KTraits, VTraits >::VerifyIntegrity() const
{
	if ((m_pRoot == NULL) || (IsNil(m_pRoot)))
		return;

	ATLASSERT(m_pRoot->m_eColor == CNode::RB_BLACK);
	int nBlackDepth = 0;
	VerifyIntegrity(m_pRoot, 0, nBlackDepth);
}

#endif // _DEBUG

template< typename K, typename V, class KTraits = WElementTraits< K >, class VTraits = WElementTraits< V > >
class WRBMap :
	public WRBTree< K, V, KTraits, VTraits >
{
public:
	explicit WRBMap( size_t nBlockSize = 10 ) throw();
	~WRBMap() throw();

	bool Lookup( KINARGTYPE key, VOUTARGTYPE value ) const; // throw( ... );
	const CPair* Lookup( KINARGTYPE key ) const throw();
	CPair* Lookup( KINARGTYPE key ) throw();
	POSITION SetAt( KINARGTYPE key, VINARGTYPE value ); // throw( ... );
	bool RemoveKey( KINARGTYPE key ) throw();
};

template< typename K, typename V, class KTraits, class VTraits >
WRBMap< K, V, KTraits, VTraits >::WRBMap( size_t nBlockSize ) throw() :
	WRBTree< K, V, KTraits, VTraits >( nBlockSize )
{
}

template< typename K, typename V, class KTraits, class VTraits >
WRBMap< K, V, KTraits, VTraits >::~WRBMap() throw()
{
}

template< typename K, typename V, class KTraits, class VTraits >
const typename WRBMap< K, V, KTraits, VTraits >::CPair* WRBMap< K, V, KTraits, VTraits >::Lookup( KINARGTYPE key ) const throw()
{
	return Find(key);
}

template< typename K, typename V, class KTraits, class VTraits >
typename WRBMap< K, V, KTraits, VTraits >::CPair* WRBMap< K, V, KTraits, VTraits >::Lookup( KINARGTYPE key ) throw()
{
	return Find(key);
}

template< typename K, typename V, class KTraits, class VTraits >
bool WRBMap< K, V, KTraits, VTraits >::Lookup( KINARGTYPE key, VOUTARGTYPE value ) const //throw( ... )
{
	const CPair* pLookup = Find( key );
	if( pLookup == NULL )
		return false;

	value = pLookup->m_value;
	return true;
}

template< typename K, typename V, class KTraits, class VTraits >
POSITION WRBMap< K, V, KTraits, VTraits >::SetAt( KINARGTYPE key, VINARGTYPE value ) //throw( ... )
{
	CPair* pNode = Find( key );
	if( pNode == NULL )
	{
		return( RBInsert( key, value ) );
	}
	else
	{
		pNode->m_value = value;

		return( pNode );
	}
}

template< typename K, typename V, class KTraits, class VTraits >
bool WRBMap< K, V, KTraits, VTraits >::RemoveKey( KINARGTYPE key ) throw()
{
	POSITION pos = Lookup( key );
	if( pos != NULL )
	{
		RemoveAt( pos );

		return( true );
	}
	else
	{
		return( false );
	}
}

template< typename K, typename V, class KTraits = WElementTraits< K >, class VTraits = WElementTraits< V > >
class WRBMultiMap :
	public WRBTree< K, V, KTraits, VTraits >
{
public:
	explicit WRBMultiMap( size_t nBlockSize = 10 ) throw();
	~WRBMultiMap() throw();

	POSITION Insert( KINARGTYPE key, VINARGTYPE value ); //throw( ... );
	size_t RemoveKey( KINARGTYPE key ) throw();

	POSITION FindFirstWithKey( KINARGTYPE key ) const throw();
	const CPair* GetNextWithKey( POSITION& pos, KINARGTYPE key ) const throw();
	CPair* GetNextWithKey( POSITION& pos, KINARGTYPE key ) throw();
	const V& GetNextValueWithKey( POSITION& pos, KINARGTYPE key ) const throw();
	V& GetNextValueWithKey( POSITION& pos, KINARGTYPE key ) throw();
};

template< typename K, typename V, class KTraits, class VTraits >
WRBMultiMap< K, V, KTraits, VTraits >::WRBMultiMap( size_t nBlockSize ) throw() :
	WRBTree< K, V, KTraits, VTraits >( nBlockSize )
{
}

template< typename K, typename V, class KTraits, class VTraits >
WRBMultiMap< K, V, KTraits, VTraits >::~WRBMultiMap() throw()
{
}

template< typename K, typename V, class KTraits, class VTraits >
POSITION WRBMultiMap< K, V, KTraits, VTraits >::Insert( KINARGTYPE key, VINARGTYPE value ) //throw( ... )
{
	return( RBInsert( key, value ) );
}

template< typename K, typename V, class KTraits, class VTraits >
size_t WRBMultiMap< K, V, KTraits, VTraits >::RemoveKey( KINARGTYPE key ) throw()
{
	size_t nElementsDeleted = 0;

	POSITION pos = FindFirstWithKey( key );
	while( pos != NULL )
	{
		POSITION posDelete = pos;
		GetNextWithKey( pos, key );
		RemoveAt( posDelete );
		nElementsDeleted++;
	}

	return( nElementsDeleted );
}

template< typename K, typename V, class KTraits, class VTraits >
POSITION WRBMultiMap< K, V, KTraits, VTraits >::FindFirstWithKey( KINARGTYPE key ) const throw()
{
	return( Find( key ) );
}

template< typename K, typename V, class KTraits, class VTraits >
const typename WRBMultiMap< K, V, KTraits, VTraits >::CPair* WRBMultiMap< K, V, KTraits, VTraits >::GetNextWithKey( POSITION& pos, KINARGTYPE key ) const throw()
{
	ATLASSERT( pos != NULL );
	const CPair* pNode = GetNext( pos );
	if( (pos == NULL) || !KTraits::CompareElements( static_cast< CPair* >( pos )->m_key, key ) )
	{
		pos = NULL;
	}

	return( pNode );
}

template< typename K, typename V, class KTraits, class VTraits >
typename WRBMultiMap< K, V, KTraits, VTraits >::CPair* WRBMultiMap< K, V, KTraits, VTraits >::GetNextWithKey( POSITION& pos, KINARGTYPE key ) throw()
{
	ATLASSERT( pos != NULL );
	CPair* pNode = GetNext( pos );
	if( (pos == NULL) || !KTraits::CompareElements( static_cast< CPair* >( pos )->m_key, key ) )
	{
		pos = NULL;
	}

	return( pNode );
}

template< typename K, typename V, class KTraits, class VTraits >
const V& WRBMultiMap< K, V, KTraits, VTraits >::GetNextValueWithKey( POSITION& pos, KINARGTYPE key ) const throw()
{
	const CPair* pPair = GetNextWithKey( pos, key );

	return( pPair->m_value );
}

template< typename K, typename V, class KTraits, class VTraits >
V& WRBMultiMap< K, V, KTraits, VTraits >::GetNextValueWithKey( POSITION& pos, KINARGTYPE key ) throw()
{
	CPair* pPair = GetNextWithKey( pos, key );

	return( pPair->m_value );
}

//#pragma pop_macro("new")

