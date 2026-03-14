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


def powerlaw_brighten(img):
    gamma = 0.95
    num_rows = img.shape[0]
    num_cols = img.shape[1]

    temp = np.zeros((num_rows, num_cols), dtype=np.float32)
    for i in range(num_rows):
        for j in range(num_cols):
            temp[i][j] = img[i][j] / 255.0

    result = np.zeros((num_rows, num_cols), dtype=np.float32)
    for i in range(num_rows):
        for j in range(num_cols):
            result[i][j] = temp[i][j] ** gamma
            result[i][j] = result[i][j] * 255.0
            if result[i][j] < 0:
                result[i][j] = 0
            if result[i][j] > 255:
                result[i][j] = 255

    return result.astype(np.uint8)


def laplacian_sharpen(img):
    kernel = [
        [-1, -1, -1],
        [-1,  9, -1],
        [-1, -1, -1]
    ]
    k = 3
    half_k = k // 2
    num_rows = img.shape[0]
    num_cols = img.shape[1]

    temp = img.astype(np.float32)
    padded = np.pad(temp, half_k, mode='edge')
    lap = np.zeros((num_rows, num_cols), dtype=np.float32)

    for i in range(num_rows):
        for j in range(num_cols):
            total = 0.0
            for ki in range(k):
                for kj in range(k):
                    total += padded[i+ki][j+kj] * kernel[ki][kj]
            lap[i][j] = total

    result = np.zeros((num_rows, num_cols), dtype=np.float32)
    for i in range(num_rows):
        for j in range(num_cols):
            val = temp[i][j] - lap[i][j]
            if val < 0:
                val = 0
            if val > 255:
                val = 255
            result[i][j] = val

    return result.astype(np.uint8)


def global_threshold(img):
    T = 150
    num_rows = img.shape[0]
    num_cols = img.shape[1]
    binary = np.zeros((num_rows, num_cols), dtype=np.uint8)

    for i in range(num_rows):
        for j in range(num_cols):
            if img[i][j] > T:
                binary[i][j] = 255

    return binary


if __name__ == "__main__":
    img = cv2.imread("../Data/Q2b.png", cv2.IMREAD_GRAYSCALE)
    img = apply_median_filter(img)
    img = powerlaw_brighten(img)
    img = apply_median_filter(img)
    img = powerlaw_brighten(img)
#    img = img + laplacian_sharpen(img)
    img = global_threshold(img)

    cv2.imwrite("../Outputs/output_2b.png", img)