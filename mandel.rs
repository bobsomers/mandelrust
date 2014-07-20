static TILE_WIDTH: int = 8i;
static TILE_HEIGHT: int = 8i;

struct Tile {
    i: int,
    j: int
}

struct Pixel<'a> {
    x: int,
    y: int
}

#[deriving(Clone)]
struct Rgb {
    r: f32,
    g: f32,
    b: f32
}

struct Window {
    x0: f32,
    x1: f32,
    y0: f32,
    y1: f32
}

struct Options<'a> {
    width_in_pixels: int,
    height_in_pixels: int,
    iterations: int,
    window: Window,
    sample_offsets: Vec<(f32, f32)>,
    sample_weights: Vec<f32>,
    sample_weight_sum: f32
}

fn mitchell(x: f32, b: f32, c: f32) -> f32 {
    let s = (2.0f32 * x).abs();

    let poly: f32 = if s < 1.0 {
        (12.0 - 9.0*b - 6.0*c) * s*s*s + (-18.0 + 12.0*b + 6.0*c) * s*s + (6.0 - 2.0*b)
    } else {
        (-b - 6.0*c) * s*s*s + (6.0*b + 30.0*c) * s*s + (-12.0*b - 48.0*c) * s + (8.0*b + 24.0*c)
    };

    poly * (1.0f32 / 6.0f32)
}

fn mitchell_weight(x: f32, y: f32, size: f32) -> f32 {
    let one_over_size = 1.0f32 / size;
    let one_third = 1.0f32 / 3.0f32;

    let mitchell_x = mitchell(x * one_over_size, one_third, one_third);
    let mitchell_y = mitchell(y * one_over_size, one_third, one_third);

    mitchell_x * mitchell_y
}

fn halton(index: int, base: int) -> f32 {
    let mut result = 0.0f32;
    let mut f = 1.0f32 / base as f32;
    let mut i = index;

    while i > 0 {
        result = result + f * (i % base) as f32;
        i = (i as f32 / base as f32).floor() as int;
        f = f / base as f32;
    }

    result
}

fn halton23(index: int) -> (f32, f32) {
    (halton(index, 2), halton(index, 3))
}

fn mandel(c_real: f32, c_imag: f32, iterations: int) -> int {
    let mut z_real = c_real;
    let mut z_imag = c_imag;
    let mut iter = 0i;

    for i in range(0i, iterations) {
        iter = i;

        if z_real * z_real + z_imag * z_imag > 4.0f32 {
            break;
        }

        let new_real = z_real * z_real - z_imag * z_imag;
        let new_imag = 2.0f32 * z_real * z_imag;

        z_real = c_real + new_real;
        z_imag = c_imag + new_imag;
    }

    iter
}

fn shade(val: int, max: int) -> Rgb {
    let v = if val < max - 1 {
        val as f32 / max as f32
    } else {
        0.0
    };

    Rgb{r: v, g: v, b: v}
}

fn pixel(pixel: Pixel, opts: &Options, buf: &mut Vec<Rgb>) {
    if pixel.x >= opts.width_in_pixels { return; }
    if pixel.y >= opts.height_in_pixels { return; }

    let center_x = pixel.x as f32 + 0.5;
    let center_y = pixel.y as f32 + 0.5;
    let window_width = opts.window.x1 - opts.window.x0;
    let window_height = opts.window.y1 - opts.window.y0;

    let mut accum = Rgb{r: 0.0, g: 0.0, b:0.0};

    for sample_index in range(0u, opts.sample_offsets.len()) {
        let (offset_x, offset_y) = opts.sample_offsets[sample_index];

        // Map sample to window space.
        let x = (center_x + offset_x) / (opts.width_in_pixels as f32) * window_width + opts.window.x0;
        let y = (center_y + offset_y) / (opts.height_in_pixels as f32) * window_height + opts.window.y0;

        // Compute Mandelbrot set under the sample and shade it.
        let val = mandel(x, y, opts.iterations);
        let rgb = shade(val, opts.iterations);
    
        // Accumulate a weighted sum of shaded samples using the precomputed
        // sample weights from a Mitchell-Netravali filter.
        let sample_weight = opts.sample_weights[sample_index];
        accum.r += rgb.r * sample_weight;
        accum.g += rgb.g * sample_weight;
        accum.b += rgb.b * sample_weight;
    }

    // Compute weighted average from weighted sum.
    let one_over_weight_sum = 1.0f32 / opts.sample_weight_sum;
    let weighted_average = Rgb {
        r: accum.r * one_over_weight_sum,
        g: accum.g * one_over_weight_sum,
        b: accum.b * one_over_weight_sum
    };

    // Write the weighted average into the buffer.
    let index = ((pixel.y * opts.width_in_pixels) + pixel.x) as uint;
    *buf.get_mut(index) = weighted_average;
}

fn tile(tile: Tile, opts: &Options, buf: &mut Vec<Rgb>) {
    let offset_x = tile.i * TILE_WIDTH;
    let offset_y = tile.j * TILE_HEIGHT;

    for ty in range(0i, TILE_HEIGHT) {
        let y = offset_y + ty;
        for tx in range(0i, TILE_WIDTH) {
            let x = offset_x + tx;
            pixel(Pixel{x: x, y: y}, opts, buf);
        }
    }
}

fn mandelbrot(opts: &Options, buf: &mut Vec<Rgb>) {
    let width_in_tiles = opts.width_in_pixels / TILE_WIDTH +
        if opts.width_in_pixels % TILE_WIDTH > 0 {
            1
        } else {
            0
        };
    let height_in_tiles = opts.height_in_pixels / TILE_HEIGHT +
        if opts.height_in_pixels % TILE_HEIGHT > 0 {
            1
        } else {
            0
        };

    for j in range(0i, height_in_tiles) {
        for i in range(0i, width_in_tiles) {
            tile(Tile {i: i, j: j}, opts, buf);
        }
    }
}

fn write(opts: &Options, buf: &Vec<Rgb>) {
    println!("P3 {} {} {}", opts.width_in_pixels, opts.height_in_pixels, 255u);

    for y in range(0i, opts.height_in_pixels) {
        for x in range(0i, opts.width_in_pixels) {
            let index = (y * opts.width_in_pixels + x) as uint;
            let rgb = buf[index];

            // Gamma correction.
            let one_over_gamma = 1.0f32 / 2.2f32;
            let r8 = (rgb.r.powf(one_over_gamma) * 255.0) as u8;
            let g8 = (rgb.g.powf(one_over_gamma) * 255.0) as u8;
            let b8 = (rgb.b.powf(one_over_gamma) * 255.0) as u8;

            print!("{} {} {} ", r8, g8, b8);
        }
        print!("\n");
    }
}

fn main() {
    let width = 640i;
    let height = 480i;
    let samples = 64i;
    let iterations = 256i;
    let filter_size = 2i;
    let window = Window {
        x0: -2.0, x1: 1.0,
        y0: -1.0, y1: 1.0
    };

    // Precompute subpixel sample offsets.
    let mut sample_offsets: Vec<(f32, f32)> = Vec::with_capacity(samples as uint);
    for i in range(0i, samples) {
        let (x, y) = halton23(i);
        sample_offsets.push((
            (x - 0.5) * filter_size as f32,
            (y - 0.5) * filter_size as f32
        ));
    }

    // Precompute subpixel sample weights (using Mitchell-Netravali filter).
    let mut sample_weights: Vec<f32> = Vec::with_capacity(samples as uint);
    let mut sample_weight_sum = 0.0f32;
    for i in range(0i, samples) {
        let (x, y) = sample_offsets[i as uint];
        let weight = mitchell_weight(x, y, filter_size as f32);
        sample_weight_sum += weight;
        sample_weights.push(mitchell_weight(x, y, filter_size as f32));
    }

    // Allocate image buffer.
    let num_pixels = (width * height) as uint;
    let mut buf: Vec<Rgb> = Vec::from_elem(num_pixels, Rgb{r: 0.0, g: 0.0, b: 0.0});

    let opts = Options {
        width_in_pixels: width,
        height_in_pixels: height,
        iterations: iterations,
        window: window,
        sample_offsets: sample_offsets,
        sample_weights: sample_weights,
        sample_weight_sum: sample_weight_sum
    };

    // Compute image.
    mandelbrot(&opts, &mut buf);

    // Write PPM format to standard out.
    write(&opts, &buf);
}
