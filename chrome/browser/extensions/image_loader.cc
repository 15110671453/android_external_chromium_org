// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/image_loader.h"

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/file_util.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/sequenced_worker_pool.h"
#include "chrome/browser/extensions/image_loader_factory.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/extensions/extension.h"
#include "content/public/browser/browser_thread.h"
#include "grit/chrome_unscaled_resources.h"
#include "grit/component_extension_resources_map.h"
#include "grit/keyboard_resources.h"
#include "grit/theme_resources.h"
#include "skia/ext/image_operations.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image_skia.h"
#include "webkit/glue/image_decoder.h"

#if defined(USE_AURA)
#include "grit/keyboard_resources.h"
#endif

using content::BrowserThread;
using extensions::Extension;
using extensions::ImageLoader;
using extensions::Manifest;

namespace {

bool ShouldResizeImageRepresentation(
    ImageLoader::ImageRepresentation::ResizeCondition resize_method,
    const gfx::Size& decoded_size,
    const gfx::Size& desired_size) {
  switch (resize_method) {
    case ImageLoader::ImageRepresentation::ALWAYS_RESIZE:
      return decoded_size != desired_size;
    case ImageLoader::ImageRepresentation::RESIZE_WHEN_LARGER:
      return decoded_size.width() > desired_size.width() ||
             decoded_size.height() > desired_size.height();
    default:
      NOTREACHED();
      return false;
  }
}

SkBitmap ResizeIfNeeded(const SkBitmap& bitmap,
                        const ImageLoader::ImageRepresentation& image_info) {
  gfx::Size original_size(bitmap.width(), bitmap.height());
  if (ShouldResizeImageRepresentation(image_info.resize_condition,
                                      original_size,
                                      image_info.desired_size)) {
    return skia::ImageOperations::Resize(
        bitmap, skia::ImageOperations::RESIZE_LANCZOS3,
        image_info.desired_size.width(), image_info.desired_size.height());
  }

  return bitmap;
}

void LoadResourceOnUIThread(int resource_id, SkBitmap* bitmap) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  gfx::ImageSkia image(
      *ResourceBundle::GetSharedInstance().GetImageSkiaNamed(resource_id));
  image.MakeThreadSafe();
  *bitmap = *image.bitmap();
}

void LoadImageOnBlockingPool(const ImageLoader::ImageRepresentation& image_info,
                             SkBitmap* bitmap) {
  DCHECK(BrowserThread::GetBlockingPool()->RunsTasksOnCurrentThread());

  // Read the file from disk.
  std::string file_contents;
  base::FilePath path = image_info.resource.GetFilePath();
  if (path.empty() || !file_util::ReadFileToString(path, &file_contents)) {
    return;
  }

  // Decode the bitmap using WebKit's image decoder.
  const unsigned char* data =
      reinterpret_cast<const unsigned char*>(file_contents.data());
  webkit_glue::ImageDecoder decoder;
  // Note: This class only decodes bitmaps from extension resources. Chrome
  // doesn't (for security reasons) directly load extension resources provided
  // by the extension author, but instead decodes them in a separate
  // locked-down utility process. Only if the decoding succeeds is the image
  // saved from memory to disk and subsequently used in the Chrome UI.
  // Chrome is therefore decoding bitmaps here that were generated by Chrome.
  *bitmap = decoder.Decode(data, file_contents.length());
}

}  // namespace

namespace extensions {

////////////////////////////////////////////////////////////////////////////////
// ImageLoader::ImageRepresentation

ImageLoader::ImageRepresentation::ImageRepresentation(
    const ExtensionResource& resource,
    ResizeCondition resize_condition,
    const gfx::Size& desired_size,
    ui::ScaleFactor scale_factor)
    : resource(resource),
      resize_condition(resize_condition),
      desired_size(desired_size),
      scale_factor(scale_factor) {
}

ImageLoader::ImageRepresentation::~ImageRepresentation() {
}

////////////////////////////////////////////////////////////////////////////////
// ImageLoader::LoadResult

struct ImageLoader::LoadResult  {
  LoadResult(const SkBitmap& bitmap,
             const gfx::Size& original_size,
             const ImageRepresentation& image_representation);
  ~LoadResult();

  SkBitmap bitmap;
  gfx::Size original_size;
  ImageRepresentation image_representation;
};

ImageLoader::LoadResult::LoadResult(
    const SkBitmap& bitmap,
    const gfx::Size& original_size,
    const ImageLoader::ImageRepresentation& image_representation)
    : bitmap(bitmap),
      original_size(original_size),
      image_representation(image_representation) {
}

ImageLoader::LoadResult::~LoadResult() {
}

////////////////////////////////////////////////////////////////////////////////
// ImageLoader

ImageLoader::ImageLoader()
    : ALLOW_THIS_IN_INITIALIZER_LIST(weak_ptr_factory_(this)) {
}

ImageLoader::~ImageLoader() {
}

// static
ImageLoader* ImageLoader::Get(Profile* profile) {
  return ImageLoaderFactory::GetForProfile(profile);
}

// static
bool ImageLoader::IsComponentExtensionResource(
    const base::FilePath& extension_path,
    const base::FilePath& resource_path,
    int* resource_id) {
  static const GritResourceMap kExtraComponentExtensionResources[] = {
    {"web_store/webstore_icon_128.png", IDR_WEBSTORE_ICON},
    {"web_store/webstore_icon_16.png", IDR_WEBSTORE_ICON_16},
    {"chrome_app/product_logo_128.png", IDR_PRODUCT_LOGO_128},
    {"chrome_app/product_logo_16.png", IDR_PRODUCT_LOGO_16},
#if defined(ENABLE_SETTINGS_APP)
    {"settings_app/settings_app_icon_128.png", IDR_SETTINGS_APP_ICON_128},
    {"settings_app/settings_app_icon_16.png", IDR_SETTINGS_APP_ICON_16},
    {"settings_app/settings_app_icon_32.png", IDR_SETTINGS_APP_ICON_32},
    {"settings_app/settings_app_icon_48.png", IDR_SETTINGS_APP_ICON_48},
#endif
#if defined(USE_AURA)
    {"keyboard/index.html", IDR_KEYBOARD_INDEX},
    {"keyboard/main.css", IDR_KEYBOARD_MAIN_CSS},
    {"keyboard/main.js", IDR_KEYBOARD_MAIN_JS},
    {"keyboard/common.js", IDR_KEYBOARD_COMMON_JS},
    {"keyboard/layout_us.js", IDR_KEYBOARD_LAYOUT_US_JS},
    {"keyboard/images/chevron.svg", IDR_KEYBOARD_IMAGES_CHEVRON},
    {"keyboard/images/del.svg", IDR_KEYBOARD_IMAGES_DEL},
    {"keyboard/images/keyboard.svg", IDR_KEYBOARD_IMAGES_KEYBOARD},
    {"keyboard/images/mic.svg", IDR_KEYBOARD_IMAGES_MIC},
    {"keyboard/images/ret.svg", IDR_KEYBOARD_IMAGES_RET},
    {"keyboard/images/shift_down.svg", IDR_KEYBOARD_IMAGES_SHIFT_DOWN},
    {"keyboard/images/shift.svg", IDR_KEYBOARD_IMAGES_SHIFT},
    {"keyboard/images/tab.svg", IDR_KEYBOARD_IMAGES_TAB},
#endif
  };
  static const size_t kExtraComponentExtensionResourcesSize =
      arraysize(kExtraComponentExtensionResources);

  base::FilePath directory_path = extension_path;
  base::FilePath resources_dir;
  base::FilePath relative_path;
  if (!PathService::Get(chrome::DIR_RESOURCES, &resources_dir) ||
      !resources_dir.AppendRelativePath(directory_path, &relative_path)) {
    return false;
  }
  relative_path = relative_path.Append(resource_path);
  relative_path = relative_path.NormalizePathSeparators();

  // TODO(tc): Make a map of base::FilePath -> resource ids so we don't have to
  // covert to FilePaths all the time.  This will be more useful as we add
  // more resources.
  for (size_t i = 0; i < kComponentExtensionResourcesSize; ++i) {
    base::FilePath resource_path =
        base::FilePath().AppendASCII(kComponentExtensionResources[i].name);
    resource_path = resource_path.NormalizePathSeparators();

    if (relative_path == resource_path) {
      *resource_id = kComponentExtensionResources[i].value;
      return true;
    }
  }
  for (size_t i = 0; i < kExtraComponentExtensionResourcesSize; ++i) {
    base::FilePath resource_path =
        base::FilePath().AppendASCII(kExtraComponentExtensionResources[i].name);
    resource_path = resource_path.NormalizePathSeparators();

    if (relative_path == resource_path) {
      *resource_id = kExtraComponentExtensionResources[i].value;
      return true;
    }
  }
  return false;
}

void ImageLoader::LoadImageAsync(
    const Extension* extension,
    const ExtensionResource& resource,
    const gfx::Size& max_size,
    const base::Callback<void(const gfx::Image&)>& callback) {
  std::vector<ImageRepresentation> info_list;
  info_list.push_back(ImageRepresentation(
      resource,
      ImageRepresentation::RESIZE_WHEN_LARGER,
      max_size,
      ui::SCALE_FACTOR_100P));
  LoadImagesAsync(extension, info_list, callback);
}

void ImageLoader::LoadImagesAsync(
    const Extension* extension,
    const std::vector<ImageRepresentation>& info_list,
    const base::Callback<void(const gfx::Image&)>& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  // Loading an image from the cache and loading resources have to happen
  // on the UI thread. So do those two things first, and pass the rest of the
  // work of as a blocking pool task.

  std::vector<SkBitmap> bitmaps;
  bitmaps.resize(info_list.size());

  int i = 0;
  for (std::vector<ImageRepresentation>::const_iterator it = info_list.begin();
       it != info_list.end(); ++it, ++i) {
    DCHECK(it->resource.relative_path().empty() ||
           extension->path() == it->resource.extension_root());

    int resource_id;
    if (extension->location() == Manifest::COMPONENT &&
        IsComponentExtensionResource(extension->path(),
                                     it->resource.relative_path(),
                                     &resource_id)) {
      LoadResourceOnUIThread(resource_id, &bitmaps[i]);
    }
  }

  DCHECK(!BrowserThread::GetBlockingPool()->RunsTasksOnCurrentThread());
  std::vector<LoadResult>* load_result = new std::vector<LoadResult>;
  BrowserThread::PostBlockingPoolTaskAndReply(
      FROM_HERE,
      base::Bind(LoadImagesOnBlockingPool, info_list, bitmaps, load_result),
      base::Bind(&ImageLoader::ReplyBack, weak_ptr_factory_.GetWeakPtr(),
                 base::Owned(load_result), callback));
}

// static
void ImageLoader::LoadImagesOnBlockingPool(
    const std::vector<ImageRepresentation>& info_list,
    const std::vector<SkBitmap>& bitmaps,
    std::vector<LoadResult>* load_result) {
  DCHECK(BrowserThread::GetBlockingPool()->RunsTasksOnCurrentThread());

  int i = 0;
  for (std::vector<ImageRepresentation>::const_iterator it = info_list.begin();
       it != info_list.end(); ++it, ++i) {
    // If we don't have a path there isn't anything we can do, just skip it.
    if (it->resource.relative_path().empty())
      continue;

    SkBitmap bitmap;
    if (!bitmaps[i].isNull()) {
      bitmap = bitmaps[i];
    } else {
      LoadImageOnBlockingPool(*it, &bitmap);
    }

    // If the image failed to load, skip it.
    if (bitmap.isNull() || bitmap.empty())
      continue;

    gfx::Size original_size(bitmap.width(), bitmap.height());
    bitmap = ResizeIfNeeded(bitmap, *it);

    load_result->push_back(LoadResult(bitmap, original_size, *it));
  }
}

void ImageLoader::ReplyBack(
    const std::vector<LoadResult>* load_result,
    const base::Callback<void(const gfx::Image&)>& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  gfx::ImageSkia image_skia;

  for (std::vector<LoadResult>::const_iterator it = load_result->begin();
       it != load_result->end(); ++it) {
    const SkBitmap& bitmap = it->bitmap;
    const ImageRepresentation& image_rep = it->image_representation;

    image_skia.AddRepresentation(gfx::ImageSkiaRep(
        bitmap, image_rep.scale_factor));
  }

  gfx::Image image;
  if (!image_skia.isNull()) {
    image_skia.MakeThreadSafe();
    image = gfx::Image(image_skia);
  }

  callback.Run(image);
}

}  // namespace extensions
