#!/usr/bin/env python3
"""Generate every Suborno icon asset from one square source artwork.

Usage:
    python3 scripts/generate_icon_assets.py path/to/source.jpg

The source must be a square raster image (JPEG or PNG, 1024x1024
recommended). A near-white background connected to the image border is
made transparent, the artwork is cropped to its content, and the outputs
below are (re)written:

    src/platform/windows/resources/suborno.ico   16/24/32/48/64/128/256
    packaging/linux/icons/org.subornoIME-{s}.png 16/24/32/48/64/128/256/512
    assets/suborno-logo.png                      512

The .ico is referenced by the Windows resource template
(src/platform/windows/resources/suborno.rc.in), so suborno_app.exe,
suborno_windows_tsf.dll, the tray/About/settings icons and the InnoSetup
installer all pick the artwork up from this single file.
"""

from __future__ import annotations

import sys
from collections import deque
from pathlib import Path

from PIL import Image, ImageFilter

PROJECT_ROOT = Path(__file__).resolve().parent.parent

ICO_PATH = PROJECT_ROOT / "src/platform/windows/resources/suborno.ico"
ICO_SIZES = [16, 24, 32, 48, 64, 128, 256]

PNG_DIR = PROJECT_ROOT / "packaging/linux/icons"
PNG_SIZES = [16, 24, 32, 48, 64, 128, 256, 512]
PNG_NAME = "org.subornoIME-{size}.png"

LOGO_PATH = PROJECT_ROOT / "assets/suborno-logo.png"
LOGO_SIZE = 512

# A border-connected pixel counts as background when it is near-white and
# not strongly colored; JPEG artifacts around the rounded corners need the
# spread tolerance to be forgiving but not so loose it eats the artwork.
WHITE_THRESHOLD = 235
CHANNEL_SPREAD = 18


def remove_background(source: Image.Image) -> Image.Image:
    """Flood-fill the border-connected near-white region to transparency."""
    rgb = source.convert("RGB")
    width, height = rgb.size
    pixels = rgb.load()

    def is_background(x: int, y: int) -> bool:
        r, g, b = pixels[x, y]
        return min(r, g, b) >= WHITE_THRESHOLD and (max(r, g, b) - min(r, g, b)) <= CHANNEL_SPREAD

    visited = bytearray(width * height)
    queue: deque[tuple[int, int]] = deque()
    for x in range(width):
        for y in (0, height - 1):
            if is_background(x, y) and not visited[y * width + x]:
                visited[y * width + x] = 1
                queue.append((x, y))
    for y in range(height):
        for x in (0, width - 1):
            if is_background(x, y) and not visited[y * width + x]:
                visited[y * width + x] = 1
                queue.append((x, y))
    while queue:
        x, y = queue.popleft()
        for nx, ny in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)):
            if 0 <= nx < width and 0 <= ny < height:
                index = ny * width + nx
                if not visited[index] and is_background(nx, ny):
                    visited[index] = 1
                    queue.append((nx, ny))

    mask = Image.frombytes("L", (width, height), bytes(0 if v else 255 for v in visited))
    # Feather the cut edge so downscales keep smooth anti-aliased borders.
    mask = mask.filter(ImageFilter.GaussianBlur(1.2))
    rgba = rgb.convert("RGBA")
    rgba.putalpha(mask)
    return rgba


def square_crop(image: Image.Image) -> Image.Image:
    """Crop to visible content, then pad to a centered square canvas."""
    bbox = image.getchannel("A").point(lambda a: 255 if a > 8 else 0).getbbox()
    if bbox is not None:
        image = image.crop(bbox)
    side = max(image.size)
    canvas = Image.new("RGBA", (side, side), (0, 0, 0, 0))
    canvas.paste(image, ((side - image.width) // 2, (side - image.height) // 2), image)
    return canvas


def verify(path: Path, expected_sizes: list[int]) -> None:
    if path.suffix == ".ico":
        icon = Image.open(path)
        present = sorted(size[0] for size in icon.info.get("sizes", ()))
        assert present == sorted(expected_sizes), f"{path}: sizes {present}"
        print(f"  {path.relative_to(PROJECT_ROOT)}: ICO sizes {present}")
    else:
        png = Image.open(path)
        assert png.size[0] == png.size[1], f"{path}: not square"
        alpha = png.getchannel("A")
        histogram = alpha.histogram()
        opaque = sum(histogram[255:]) / (png.width * png.height)
        transparent = sum(histogram[:1]) / (png.width * png.height)
        assert 0.05 < opaque < 0.99, f"{path}: flat alpha {opaque:.2f}"
        print(
            f"  {path.relative_to(PROJECT_ROOT)}: {png.width}x{png.height}"
            f" opaque {opaque:.2f} transparent {transparent:.2f}"
        )


def main() -> int:
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    source_path = Path(sys.argv[1])
    if not source_path.is_file():
        print(f"ERROR: source image not found: {source_path}", file=sys.stderr)
        return 1
    source = Image.open(source_path)
    if source.size[0] != source.size[1]:
        print(f"ERROR: source must be square, got {source.size}", file=sys.stderr)
        return 1

    master = square_crop(remove_background(source))
    largest = max(ICO_SIZES[-1], max(PNG_SIZES), LOGO_SIZE)
    if master.size[0] < largest:
        master = master.resize((largest, largest), Image.LANCZOS)

    print("Writing Windows .ico:")
    frames = [master.resize((size, size), Image.LANCZOS) for size in ICO_SIZES]
    ICO_PATH.parent.mkdir(parents=True, exist_ok=True)
    frames[-1].save(
        ICO_PATH,
        format="ICO",
        append_images=frames[:-1],
        sizes=[(size, size) for size in ICO_SIZES],
    )
    verify(ICO_PATH, ICO_SIZES)

    print("Writing Linux hicolor PNGs:")
    PNG_DIR.mkdir(parents=True, exist_ok=True)
    for size in PNG_SIZES:
        png_path = PNG_DIR / PNG_NAME.format(size=size)
        master.resize((size, size), Image.LANCZOS).save(png_path, format="PNG")
        verify(png_path, [size])

    print("Writing README logo:")
    LOGO_PATH.parent.mkdir(parents=True, exist_ok=True)
    master.resize((LOGO_SIZE, LOGO_SIZE), Image.LANCZOS).save(LOGO_PATH, format="PNG")
    verify(LOGO_PATH, [LOGO_SIZE])
    print("Done.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
