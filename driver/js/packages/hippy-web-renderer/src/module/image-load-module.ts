/*
 * Tencent is pleased to support the open source community by making
 * Hippy available.
 *
 * Copyright (C) 2022 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { HippyWebModule } from '../base';
import { HippyCallBack } from '../types';

export class ImageLoadModule extends HippyWebModule {
  public name = 'ImageLoaderModule';

  public getSize(url: string, callBack: HippyCallBack) {
    if (!url) {
      callBack.reject(`image url not support ${url}`);
      return;
    }

    const img = new Image();
    img.onload = () => {
      callBack.resolve({ width: img.width, height: img.height });
    };
    img.src = url;
  }

  public prefetch(url: string) {
    if (!url) {
      return;
    }
    const img = new Image();
    img.src = url;
  }

  public initialize() {

  }

  public destroy() {

  }
}