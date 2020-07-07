// Copyright 2019 - 2020 Esri. All Rights Reserved.

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
