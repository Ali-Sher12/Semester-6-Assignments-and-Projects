import cv2
import numpy as np


def apply_median_filter(img):
    k = 3
    half_k = k // 2
    num_rows = img.shape[0]
    num_cols = img.shape[1]

    padded = np.pad(img, half_k, mode='edge')
    output = np.zeros((num_rows, num_cols), dtype=np.float64)

    for i in range(num_rows):
        for j in range(num_cols):
            patch = padded[i:i+k, j:j+k]
            vals = []
            for pi in range(k):
                for pj in range(k):
                    vals.append(patch[pi][pj])
            vals.sort()
            output[i][j] = vals[len(vals) // 2]

    return output


def expand_mask(mask, num_iter=2):
    temp = mask.copy()
    num_rows = temp.shape[0]
    num_cols = temp.shape[1]

    for itr in range(num_iter):
        new_mask = np.zeros((num_rows, num_cols), dtype=np.uint8)
        for i in range(num_rows):
            for j in range(num_cols):
                is_set = 0
                if temp[i][j] > 0:
                    is_set = 1
                else:
                    for di in range(-1, 2):
                        for dj in range(-1, 2):
                            if di == 0 and dj == 0:
                                continue
                            ni = i + di
                            nj = j + dj
                            if 0 <= ni < num_rows and 0 <= nj < num_cols:
                                if temp[ni][nj] > 0:
                                    is_set = 1
                if is_set == 1:
                    new_mask[i][j] = 255
        temp = new_mask

    return temp


def apply_box_filter(img, k=3):
    half_k = k // 2
    num_rows = img.shape[0]
    num_cols = img.shape[1]
    padded = np.pad(img.astype(np.float64), half_k, mode='edge')
    output = np.zeros((num_rows, num_cols), dtype=np.float64)

    for i in range(num_rows):
        for j in range(num_cols):
            total = 0.0
            for ki in range(k):
                for kj in range(k):
                    total += padded[i+ki][j+kj]
            output[i][j] = total / (k * k)

    for i in range(num_rows):
        for j in range(num_cols):
            if output[i][j] < 0:
                output[i][j] = 0
            if output[i][j] > 255:
                output[i][j] = 255

    return output.astype(np.uint8)


def smooth_masked_region(img, mask, k=3):
    half_k = k // 2
    num_rows = img.shape[0]
    num_cols = img.shape[1]
    padded = np.pad(img.astype(np.float64), half_k, mode='edge')
    result = img.copy().astype(np.float64)

    for i in range(num_rows):
        for j in range(num_cols):
            if mask[i][j] > 0:
                total = 0.0
                for ki in range(k):
                    for kj in range(k):
                        total += padded[i+ki][j+kj]
                result[i][j] = total / (k * k)

    for i in range(num_rows):
        for j in range(num_cols):
            if result[i][j] < 0:
                result[i][j] = 0
            if result[i][j] > 255:
                result[i][j] = 255

    return result.astype(np.uint8)


def find_watermark(img, k=21, thresh=10):
    half_k = k // 2
    padded = np.pad(img.astype(np.float64), half_k, mode='reflect')
    num_rows = img.shape[0]
    num_cols = img.shape[1]
    mask = np.zeros((num_rows, num_cols), dtype=np.uint8)

    for i in range(num_rows):
        for j in range(num_cols):
            total = 0.0
            for ki in range(k):
                for kj in range(k):
                    total += padded[i+ki][j+kj]
            local_avg = total / (k * k)

            diff = img[i][j] - local_avg
            if diff < 0:
                diff = diff * -1

            if diff > thresh:
                mask[i][j] = 255

    return mask


def remove_watermark(img, mask, k=15):
    half_k = k // 2
    num_rows = img.shape[0]
    num_cols = img.shape[1]
    padded_img = np.pad(img.astype(np.float64), half_k, mode='reflect')
    padded_mask = np.pad(mask, half_k, mode='reflect')
    result = img.copy().astype(np.float64)

    # first estimate global background from all non-watermark pixels
    total_bg = 0.0
    count_bg = 0
    for i in range(num_rows):
        for j in range(num_cols):
            if mask[i][j] == 0:
                total_bg += img[i][j]
                count_bg += 1

    global_bg = 0.0
    if count_bg > 0:
        global_bg = total_bg / count_bg

    for i in range(num_rows):
        for j in range(num_cols):
            if mask[i][j] > 0:
                total = 0.0
                count = 0
                for ki in range(k):
                    for kj in range(k):
                        if padded_mask[i+ki][j+kj] == 0:
                            total += padded_img[i+ki][j+kj]
                            count += 1

                if count > 0:
                    result[i][j] = total / count
                else:
                    # no clean neighbors found, fall back to global bg
                    result[i][j] = global_bg

    for i in range(num_rows):
        for j in range(num_cols):
            if result[i][j] < 0:
                result[i][j] = 0
            if result[i][j] > 255:
                result[i][j] = 255

    return result.astype(np.uint8)


if __name__ == "__main__":
    img = cv2.imread("../Data/Q3.png", cv2.IMREAD_GRAYSCALE)

    mask = find_watermark(img, k=21, thresh=10)
    mask = expand_mask(mask, num_iter=2)
    mask = (mask > 0).astype(np.uint8)

    img_clean = remove_watermark(img, mask, k=15)

    # only smooth the watermark region
    img_clean = smooth_masked_region(img_clean, mask, k=3)

    cv2.imwrite("../Outputs/output_3.png", img_clean)