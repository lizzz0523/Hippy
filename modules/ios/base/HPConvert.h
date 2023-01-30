/*!
 * iOS SDK
 *
 * Tencent is pleased to support the open source community by making
 * Hippy available.
 *
 * Copyright (C) 2019 THL A29 Limited, a Tencent company.
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

#import <UIKit/UIKit.h>

#import "HPLog.h"

/**
 * This class provides a collection of conversion functions for mapping
 * JSON objects to native types and classes. These are useful when writing
 * custom setter methods.
 */

@interface HPConvert : NSObject

+ (id)id:(id)json;

+ (BOOL)BOOL:(id)json;
+ (double)double:(id)json;
+ (float)float:(id)json;
+ (int)int:(id)json;

+ (int64_t)int64_t:(id)json;
+ (uint64_t)uint64_t:(id)json;

+ (NSInteger)NSInteger:(id)json;
+ (NSUInteger)NSUInteger:(id)json;

+ (NSArray *)NSArray:(id)json;
+ (NSDictionary *)NSDictionary:(id)json;
+ (NSString *)NSString:(id)json;
+ (NSNumber *)NSNumber:(id)json;

+ (NSSet *)NSSet:(id)json;
+ (NSData *)NSData:(id)json;
+ (NSIndexSet *)NSIndexSet:(id)json;

+ (NSURL *)NSURL:(id)json;
+ (NSURLRequest *)NSURLRequest:(id)json;

+ (NSDate *)NSDate:(id)json;
+ (NSTimeZone *)NSTimeZone:(id)json;
+ (NSTimeInterval)NSTimeInterval:(id)json;

+ (NSLineBreakMode)NSLineBreakMode:(id)json;
+ (NSTextAlignment)NSTextAlignment:(id)json;
+ (NSUnderlineStyle)NSUnderlineStyle:(id)json;
+ (NSWritingDirection)NSWritingDirection:(id)json;
+ (UITextAutocapitalizationType)UITextAutocapitalizationType:(id)json;
+ (UITextFieldViewMode)UITextFieldViewMode:(id)json;
+ (UIKeyboardType)UIKeyboardType:(id)json;
+ (UIKeyboardAppearance)UIKeyboardAppearance:(id)json;
+ (UIReturnKeyType)UIReturnKeyType:(id)json;
+ (UIDataDetectorTypes)UIDataDetectorTypes:(id)json;

+ (UIViewContentMode)UIViewContentMode:(id)json;
+ (UIBarStyle)UIBarStyle:(id)json;

+ (CGFloat)CGFloat:(id)json;
+ (CGPoint)CGPoint:(id)json;
+ (CGSize)CGSize:(id)json;
+ (CGRect)CGRect:(id)json;
+ (UIEdgeInsets)UIEdgeInsets:(id)json;

+ (CGLineCap)CGLineCap:(id)json;
+ (CGLineJoin)CGLineJoin:(id)json;

//+ (CATransform3D)CATransform3D:(id)json;
+ (CGAffineTransform)CGAffineTransform:(id)json;

+ (UIColor *)UIColor:(id)json;
+ (CGColorRef)CGColor:(id)json CF_RETURNS_NOT_RETAINED;

+ (NSArray<NSArray *> *)NSArrayArray:(id)json;
+ (NSArray<NSString *> *)NSStringArray:(id)json;
+ (NSArray<NSArray<NSString *> *> *)NSStringArrayArray:(id)json;
+ (NSArray<NSDictionary *> *)NSDictionaryArray:(id)json;
+ (NSArray<NSURL *> *)NSURLArray:(id)json;
+ (NSArray<NSNumber *> *)NSNumberArray:(id)json;
+ (NSArray<UIColor *> *)UIColorArray:(id)json;

typedef NSArray CGColorArray;
+ (CGColorArray *)CGColorArray:(id)json;

/**
 * Convert a JSON object to a Plist-safe equivalent by stripping null values.
 */
typedef id NSPropertyList;
+ (NSPropertyList)NSPropertyList:(id)json;

typedef BOOL css_backface_visibility_t;
+ (css_backface_visibility_t)css_backface_visibility_t:(id)json;

@end

HP_EXTERN NSNumber *HPConvertEnumValue(const char *, NSDictionary *, NSNumber *, id);
HP_EXTERN NSNumber *HPConvertMultiEnumValue(const char *, NSDictionary *, NSNumber *, id);
HP_EXTERN NSArray *HPConvertArrayValue(SEL, id);

/**
 * Get the converter function for the specified type
 */
HP_EXTERN SEL HPConvertSelectorForType(NSString *type);

/**
 * This macro is used for logging conversion errors. This is just used to
 * avoid repeating the same boilerplate for every error message.
 */
#define HPLogConvertError(json, typeName) \
    HPLogError(@"JSON value '%@' of type %@ cannot be converted to %@", json, [json classForCoder], typeName)

/**
 * This macro is used for creating simple converter functions that just call
 * the specified getter method on the json value.
 */
#define HP_CONVERTER(type, name, getter) HP_CUSTOM_CONVERTER(type, name, [json getter])

/**
 * This macro is used for creating converter functions with arbitrary logic.
 */
#define HP_CUSTOM_CONVERTER(type, name, code)     \
    +(type)name : (id)json {                         \
        if (!HP_DEBUG) {                          \
            return code;                             \
        } else {                                     \
            @try {                                   \
                return code;                         \
            } @catch (__unused NSException * e) {    \
                HPLogConvertError(json, @ #type); \
                json = nil;                          \
                return code;                         \
            }                                        \
        }                                            \
    }

/**
 * This macro is similar to HP_CONVERTER, but specifically geared towards
 * numeric types. It will handle string input correctly, and provides more
 * detailed error reporting if an invalid value is passed in.
 */
#define HP_NUMBER_CONVERTER(type, getter) \
    HP_CUSTOM_CONVERTER(type, type, [HP_DEBUG ? [self NSNumber:json] : json getter])

/**
 * This macro is used for creating converters for enum types.
 */
#define HP_ENUM_CONVERTER(type, values, default, getter)                           \
    +(type)type : (id)json {                                                                  \
        static NSDictionary *mapping;                                                         \
        static dispatch_once_t onceToken;                                                     \
        dispatch_once(&onceToken, ^{                                                          \
            mapping = values;                                                                 \
        });                                                                                   \
        return (type)[HPConvertEnumValue(#type, mapping, @(default), json) getter]; \
    }

/**
 * This macro is used for creating converters for enum types for
 * multiple enum values combined with | operator
 */
#define HP_MULTI_ENUM_CONVERTER(type, values, default, getter)                     \
    +(type)type : (id)json {                                                                  \
        static NSDictionary *mapping;                                                         \
        static dispatch_once_t onceToken;                                                     \
        dispatch_once(&onceToken, ^{                                                          \
            mapping = values;                                                                 \
        });                                                                                   \
        return [HPConvertMultiEnumValue(#type, mapping, @(default), json) getter];  \
    }

/**
 * This macro is used for creating converter functions for typed arrays.
 */
#define HP_ARRAY_CONVERTER(type)                             \
    +(NSArray<type *> *)type##Array : (id)json {                        \
        return HPConvertArrayValue(@selector(type:), json);   \
    }
