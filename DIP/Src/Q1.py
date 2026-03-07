import cv2
import numpy as np

############################################################
# Utility filters (optional, still defined if you want them)
############################################################
def box_filter(image, k=3):
    pad = k // 2
    img = image.astype(np.float64)
    padded = np.pad(img, pad, mode='edge')
    h, w = img.shape
    out = np.zeros((h, w), dtype=np.float64)
    for i in range(k):
        for j in range(k):
            out += padded[i:i+h, j:j+w]
    return out / (k * k)

def median_filter(image, k=3):
    pad = k // 2
    img = image.astype(np.float64)
    padded = np.pad(img, pad, mode='edge')
    h, w = image.shape
    shape   = (h, w, k, k)
    strides = (padded.strides[0], padded.strides[1],
               padded.strides[0], padded.strides[1])
    windows = np.lib.stride_tricks.as_strided(padded, shape=shape, strides=strides)
    return np.median(windows.reshape(h, w, -1), axis=2)

############################################################
# Main
############################################################
if __name__ == "__main__":
    OUTPUT_PATH = "output_video_fixed.mp4"

    # Read video
    captureVideo = cv2.VideoCapture("../Data/q1.mp4")
    frames = []
    while True:
        imagePresent, image = captureVideo.read()
        if not imagePresent:
            break

        # Convert to grayscale
        if image.ndim == 3:
            gray = (0.299 * image[:,:,2] + 0.587 * image[:,:,1] + 0.114 * image[:,:,0])
            gray = gray.astype(np.uint8)
        else:
            gray = image

        frames.append(gray)

    FPS = captureVideo.get(cv2.CAP_PROP_FPS)
    W   = int(captureVideo.get(cv2.CAP_PROP_FRAME_WIDTH))
    H   = int(captureVideo.get(cv2.CAP_PROP_FRAME_HEIGHT))
    captureVideo.release()

    ############################################################
    # Compute background
    ############################################################
    background = np.median(np.array(frames, dtype=np.float64), axis=0).astype(np.uint8)

    ############################################################
    # Adaptive threshold + morphological mask for one frame
    ############################################################
    # Pick frame with maximum difference
    fluctuations = [np.median(np.abs(f.astype(float)-background.astype(float))) for f in frames]
    frame_index = np.argmax(fluctuations)
    object_frame = frames[frame_index]

    diff_abs = np.abs(object_frame.astype(np.float64) - background.astype(np.float64))

    # Adaptive threshold based on background stats
    THRESHOLD = np.mean(diff_abs) + 2*np.std(diff_abs)
    mask = (diff_abs > THRESHOLD).astype(np.uint8) * 255

    # Morphological operations to clean mask
    kernel = np.ones((5,5), np.uint8)
    mask_clean = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel)
    mask_clean = cv2.morphologyEx(mask_clean, cv2.MORPH_OPEN, kernel)

    # Apply mask
    gray_bg = np.full_like(object_frame, 128)
    result = object_frame.copy()
    result[mask_clean == 0] = gray_bg[mask_clean == 0]

    ############################################################
    # Process all frames
    ############################################################
    processed = []
    for frame in frames:
        diff_abs = np.abs(frame.astype(np.float64) - background.astype(np.float64))
        mask = (diff_abs > THRESHOLD).astype(np.uint8) * 255

        # Morphological clean
        mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel)
        mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel)

        result = frame.copy()
        result[mask == 0] = gray_bg[mask == 0]
        processed.append(result)

    ############################################################
    # Write video
    ############################################################
    fourcc = cv2.VideoWriter_fourcc(*'mp4v')
    writer = cv2.VideoWriter(OUTPUT_PATH, fourcc, FPS, (W, H), True)

    for f in processed:
        frame_bgr = np.stack([f, f, f], axis=2)
        writer.write(frame_bgr)

    writer.release()
    print("Background removed video saved to:", OUTPUT_PATH)