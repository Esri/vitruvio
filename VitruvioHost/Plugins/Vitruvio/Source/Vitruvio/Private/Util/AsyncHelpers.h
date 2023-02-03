/* Copyright 2023 Esri
 *
 * Licensed under the Apache License Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "Async/Async.h"
#include "Async/Future.h"
#include "Templates/Function.h"

namespace Vitruvio
{
template <typename ResultType, typename... Args>
static TFunction<void()> MakePromiseKeeper(const TSharedRef<TPromise<ResultType>, ESPMode::ThreadSafe>& Promise,
										   const TFunction<ResultType(Args...)>& Function)
{
	return [Promise, Function]() {
		Promise->SetValue(Function());
	};
}

template <typename... Args>
static TFunction<void()> MakePromiseKeeper(const TSharedRef<TPromise<void>, ESPMode::ThreadSafe>& Promise, const TFunction<void(Args...)>& Function)
{
	return [Promise, Function]() {
		Function();
		Promise->SetValue();
	};
}

template <typename ResultType>
static TFuture<ResultType> ExecuteOnGameThread(const TFunction<ResultType()>& Function)
{
	TSharedRef<TPromise<ResultType>, ESPMode::ThreadSafe> Promise = MakeShareable(new TPromise<ResultType>());
	TFunction<void()> PromiseKeeper = MakePromiseKeeper(Promise, Function);
	if (!IsInGameThread())
	{
		AsyncTask(ENamedThreads::GameThread, MoveTemp(PromiseKeeper));
	}
	else
	{
		PromiseKeeper();
	}
	return Promise->GetFuture();
}
} // namespace Vitruvio
