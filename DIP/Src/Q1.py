import cv2
import numpy as np

def apply_box_filter(img, k=3):
    half_k = k // 2
    num_rows = img.shape[0]
    num_cols = img.shape[1]
    padded = np.pad(img.astype(np.float64), half_k, mode='edge')
    output = np.zeros((num_rows, num_cols), dtype=np.float64)

    for i in range(k):
        for j in range(k):
            output += padded[i:i+num_rows, j:j+num_cols]
    return output / (k * k)


def apply_median_filter(img, k=3):
    half_k = k // 2
    num_rows = img.shape[0]
    num_cols = img.shape[1]
    padded = np.pad(img.astype(np.float64), half_k, mode='edge')

    patches = []
    for i in range(k):
        for j in range(k):
            patches.append(padded[i:i+num_rows, j:j+num_cols])

    stack = np.array(patches)
    stack.sort(axis=0)
    mid = (k * k) // 2
    return stack[mid]


def do_convolve(img, kernel):
    kh = kernel.shape[0]
    kw = kernel.shape[1]
    ph = kh // 2
    pw = kw // 2
    num_rows = img.shape[0]
    num_cols = img.shape[1]

    padded = np.pad(img.astype(np.float64), ((ph, ph), (pw, pw)), mode='edge')
    output = np.zeros((num_rows, num_cols), dtype=np.float64)

    for i in range(kh):
        for j in range(kw):
            output += kernel[i][j] * padded[i:i+num_rows, j:j+num_cols]

    return output

def build_mask(frame, background, lap_kernel):
    diff_abs = np.abs(frame.astype(np.float64) - background.astype(np.float64))
    frame_mean = np.sum(diff_abs) / diff_abs.size
    variance = np.sum((diff_abs - frame_mean) ** 2) / diff_abs.size
    frame_std = variance ** 0.5
    thresh = frame_mean + 2.0 * frame_std

    mask = (diff_abs > thresh).astype(np.float64) * 255.0
    mask = apply_box_filter(mask, k=5)
    mask = (mask > 127).astype(np.float64) * 255.0
    mask = apply_median_filter(mask, k=5)
    mask = (mask > 127).astype(np.float64) * 255.0
    mask = do_convolve(mask, lap_kernel)
    mask = np.clip(mask, 0, 255)

    return (mask > 127).astype(np.uint8)


if __name__ == "__main__":
    OUTPUT_PATH = "../Outputs/output_1.mp4"

    cap = cv2.VideoCapture("../Data/Q1.mp4")
    all_frames = []

    while True:
        got_frame, raw = cap.read()
        if not got_frame:
            break
        if raw.ndim == 3:
            gray = (0.299 * raw[:, :, 2] +
                    0.587 * raw[:, :, 1] +
                    0.114 * raw[:, :, 0])
            gray = gray.astype(np.uint8)
        else:
            gray = raw
        all_frames.append(gray)

    FPS = cap.get(cv2.CAP_PROP_FPS)
    VID_W = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    VID_H = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    cap.release()

    stacked = np.array(all_frames, dtype=np.float64)
    background = np.median(stacked, axis=0).astype(np.uint8)

    lap_kernel = np.array([
        [ 0, -1,  0],
        [-1,  5, -1],
        [ 0, -1,  0]
    ], dtype=np.float64)

    green_bg = np.zeros((VID_H, VID_W, 3), dtype=np.uint8)
    green_bg[:, :, 1] = 255  # green bg

    processed_frames = []

    for idx in range(len(all_frames)):
        frame = all_frames[idx]
        fg_mask = build_mask(frame, background, lap_kernel)  # 0 or 1
        frame_bgr = np.stack([frame, frame, frame], axis=2)
        result = np.where(fg_mask[:, :, np.newaxis] == 1, frame_bgr, green_bg)
        result = result.astype(np.uint8)

        processed_frames.append(result)

    fourcc = cv2.VideoWriter_fourcc(*'mp4v')
    writer = cv2.VideoWriter(OUTPUT_PATH, fourcc, FPS, (VID_W, VID_H), True)

    for f in processed_frames:
        writer.write(f)

    writer.release()