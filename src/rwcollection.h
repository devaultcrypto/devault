// Copyright (c) 2018 The Bitcoin developers
// Copyright (c) 2019 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <noncopyable.h>
#include <boost/range/iterator.hpp>

#include <shared_mutex>
#include <mutex>
#include <iterator>
#include <type_traits>
#include <utility>

template <typename T, typename L> class RWCollectionView : noncopyable {
private:
    L lock;
    T *collection;

    template <typename I> struct BracketType {
        typedef decltype(std::declval<T &>()[std::declval<I>()]) type;
    };

public:
    RWCollectionView(L l, T &c) : lock(std::move(l)), collection(&c) {}
    RWCollectionView(RWCollectionView &&other)
        : lock(std::move(other.lock)), collection(other.collection) {}

    T *operator->() { return collection; }
    const T *operator->() const { return collection; }

    /**
     * Iterator mechanics.
     */
    typedef typename boost::range_iterator<T>::type iterator;
    iterator begin() { return std::begin(*collection); }
    iterator end() { return std::end(*collection); }
    std::reverse_iterator<iterator> rbegin() {
        return std::rbegin(*collection);
    }
    std::reverse_iterator<iterator> rend() { return std::rend(*collection); }

    typedef typename boost::range_iterator<const T>::type const_iterator;
    const_iterator begin() const { return std::begin(*collection); }
    const_iterator end() const { return std::end(*collection); }
    std::reverse_iterator<const_iterator> rbegin() const {
        return std::rbegin(*collection);
    }
    std::reverse_iterator<const_iterator> rend() const {
        return std::rend(*collection);
    }

    /**
     * Forward bracket operator.
     */
    template <typename I> typename BracketType<I>::type operator[](I &&index) {
        return (*collection)[std::forward<I>(index)];
    }
};

template <typename T> class RWCollection {
private:
    mutable std::shared_mutex rwlock;
    T collection;

public:
    RWCollection() : collection() {}

    typedef RWCollectionView<const T, std::shared_lock<std::shared_mutex>> ReadView;
    ReadView getReadView() const { return ReadView(std::shared_lock<std::shared_mutex>(rwlock), collection);  }
    typedef RWCollectionView<T, std::unique_lock<std::shared_mutex>> WriteView;
    WriteView getWriteView() {
        return WriteView(std::unique_lock<std::shared_mutex>(rwlock), collection);
    }

};
