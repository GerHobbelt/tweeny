/*
 This file is part of the Tweeny library.

 Copyright (c) 2016 Leonardo G. Lucena de Freitas
 Copyright (c) 2016 Guilherme R. Costa

 Permission is hereby granted, free of charge, to any person obtaining a copy of
 this software and associated documentation files (the "Software"), to deal in
 the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 the Software, and to permit persons to whom the Software is furnished to do so,
 subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*
 * The purpose of this file is to hold implementations for the tween.h file.
 */

#ifndef TWEENY_TWEEN_TCC
#define TWEENY_TWEEN_TCC

#include "tween.h"
#include "dispatcher.h"

namespace tweeny {

    namespace detail {
        template<typename T>
        T clip(const T & n, const T & lower, const T & upper) {
            return std::max(lower, std::min(n, upper));
        }
    }

    template<typename T, typename... Ts> inline tween<T, Ts...> tween<T, Ts...>::from(T t, Ts... vs) { return tween<T, Ts...>(t, vs...); }
    template<typename T, typename... Ts> inline tween<T, Ts...>::tween() { }
    template<typename T, typename... Ts> inline tween<T, Ts...>::tween(T t, Ts... vs) {
        points.emplace_back(t, vs...);
    }

    template<typename T, typename... Ts> inline tween<T, Ts...> & tween<T, Ts...>::to(T t, Ts... vs) {
        points.emplace_back(t, vs...);
        return *this;
    }

    template<typename T, typename... Ts>
    template<typename... Fs>
    inline tween<T, Ts...> & tween<T, Ts...>::via(Fs... vs) {
        points.at(points.size() - 2).via(vs...);
        return *this;
    }

    template<typename T, typename... Ts>
    template<typename... Fs>
    inline tween<T, Ts...> & tween<T, Ts...>::via(int index, Fs... vs) {
        points.at(static_cast<size_t>(index)).via(vs...);
        return *this;
    }

    template<typename T, typename... Ts>
    template<typename... Ds>
    inline tween<T, Ts...> & tween<T, Ts...>::during(Ds... ds) {
        total = 0;
        points.at(points.size() - 2).during(ds...);
        for (detail::tweenpoint<T, Ts...> & p : points) {
            total += p.duration();
            p.stacked = total;
        }
        return *this;
    }

    template<typename T, typename... Ts>
    inline const typename detail::tweentraits<T, Ts...>::valuesType & tween<T, Ts...>::step(int32_t dt, bool suppress) {
        return step(static_cast<float>(dt * direction)/static_cast<float>(total), suppress);
    }

    template<typename T, typename... Ts>
    inline const typename detail::tweentraits<T, Ts...>::valuesType & tween<T, Ts...>::step(uint32_t dt, bool suppress) {
        return step(static_cast<int32_t>(dt), suppress);
    }

    template<typename T, typename... Ts>
    inline const typename detail::tweentraits<T, Ts...>::valuesType & tween<T, Ts...>::step(float dp, bool suppress) {
        seek(currentProgress + dp, true);
        if (!suppress) dispatch(onStepCallbacks);
        return current;
    }

    template<typename T, typename... Ts>
    inline const typename detail::tweentraits<T, Ts...>::valuesType & tween<T, Ts...>::seek(float p, bool suppress) {
        p = detail::clip(p, 0.0f, 1.0f);
        currentProgress = p;
        render(p);
        if (!suppress) dispatch(onSeekCallbacks);
        return current;
    }

    template<typename T, typename... Ts>
    inline const typename detail::tweentraits<T, Ts...>::valuesType & tween<T, Ts...>::seek(int32_t t, bool suppress) {
        return seek(static_cast<float>(t) / static_cast<float>(total), suppress);
    }

    template<typename T, typename... Ts>
    inline uint32_t tween<T, Ts...>::duration() {
        return total;
    }

    template<typename T, typename... Ts>
    template<size_t I>
    inline void tween<T, Ts...>::interpolate(float prog, detail::int2type<I>) {
        auto & p = points.at(currentPoint);
        float pointTotal = (prog * p.duration()) / p.duration(I);
        if (pointTotal > 1.0f) pointTotal = 1.0f;
        auto easing = std::get<I>(p.easings);
        std::get<I>(current) = easing(pointTotal, std::get<I>(p.values), std::get<I>(points.at(currentPoint+1).values));
        interpolate(prog, detail::int2type<I-1>{ });
    }

    template<typename T, typename... Ts>
    inline void tween<T, Ts...>::interpolate(float prog, detail::int2type<0>) {
        auto & p = points.at(currentPoint);
        float pointTotal = (prog * p.duration()) / p.duration(0);
        if (pointTotal > 1.0f) pointTotal = 1.0f;
        auto easing = std::get<0>(p.easings);
        std::get<0>(current) = easing(pointTotal, std::get<0>(p.values), std::get<0>(points.at(currentPoint+1).values));
    }

    template<typename T, typename... Ts>
    inline void tween<T, Ts...>::render(float p) {
        uint32_t t = static_cast<uint32_t>(p * total);
        if (t > points.at(currentPoint).stacked) currentPoint++;
        if (currentPoint > 0 && t <= points.at(currentPoint - 1u).stacked) currentPoint--;
        interpolate(p, detail::int2type<sizeof...(Ts) - 1 + 1 /* +1 for the T */>{ });
    }

    template<typename T, typename... Ts>
    tween<T, Ts...> & tween<T, Ts...>::onStep(typename detail::tweentraits<T, Ts...>::callbackType callback) {
        onStepCallbacks.push_back(callback);
        return *this;
    }

    template<typename T, typename... Ts>
    tween<T, Ts...> & tween<T, Ts...>::onSeek(typename detail::tweentraits<T, Ts...>::callbackType callback) {
        onSeekCallbacks.push_back(callback);
        return *this;
    }

    template<typename T, typename... Ts>
    void tween<T, Ts...>::dispatch(std::vector<typename traits::callbackType> & cbVector) {
        std::vector<size_t> dismissed;
        for (size_t i = 0; i < cbVector.size(); ++i) {
            auto && cb = cbVector[i];
            bool dismiss = detail::call<bool>(cb, std::tuple_cat(std::make_tuple(std::ref(*this)), current));
            if (dismiss) dismissed.push_back(i);
        }

        if (dismissed.size() > 0) {
            for (size_t i = 0; i < dismissed.size(); ++i) {
                size_t index = dismissed[i];
                cbVector[index] = cbVector.at(cbVector.size() - 1 - i);
            }
            cbVector.resize(cbVector.size() - dismissed.size());
        }
    }

    template<typename T, typename... Ts>
    float tween<T, Ts...>::progress() {
        return currentProgress;
    }

    template<typename T, typename... Ts>
    tween<T, Ts...> & tween<T, Ts...>::forward() {
        direction = 1;
        return *this;
    }

    template<typename T, typename... Ts>
    tween<T, Ts...> & tween<T, Ts...>::backward() {
        direction = -1;
        return *this;
    }

    template<typename T, typename... Ts>
    inline const typename detail::tweentraits<T, Ts...>::valuesType & tween<T, Ts...>::jump(int32_t p, bool suppress) {
        p = detail::clip(p, 0, points.size() -1);
        return seek(points.at(p).stacked, suppress);
    }

    template<typename T, typename... Ts> inline uint16_t tween<T, Ts...>::point() { return currentPoint; };
}

#endif //TWEENY_TWEEN_TCC