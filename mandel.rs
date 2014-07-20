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

struct Options {
    width_in_pixels: int,
    height_in_pixels: int,
    iterations: int,
    window: Window
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

fn shade(x: f32, y: f32, w:f32, h: f32, val: int, max: int) -> Rgb {
    let v = val as f32 / max as f32;
    Rgb{r: v, g: v, b: v}
}

fn pixel(pixel: Pixel, opts: &Options, buf: &mut Vec<Rgb>) {
    if pixel.x >= opts.width_in_pixels { return; }
    if pixel.y >= opts.height_in_pixels { return; }

    let center_x = pixel.x as f32 + 0.5;
    let center_y = pixel.y as f32 + 0.5;

    // Map pixel center to window space.
    let window_width = opts.window.x1 - opts.window.x0;
    let window_height = opts.window.y1 - opts.window.y0;
    let x = center_x / (opts.width_in_pixels as f32) * window_width + opts.window.x0;
    let y = center_y / (opts.height_in_pixels as f32) * window_height + opts.window.y0;

    let val = mandel(x, y, opts.iterations);
    let rgb = shade(x, y, window_width, window_height, val, opts.iterations);

    let index = ((pixel.y * opts.width_in_pixels) + pixel.x) as uint;
    *buf.get_mut(index) = rgb;

    //println!("\tpixel = <{}, {}>, index = {}", pixel.x, pixel.y, index);
}

fn tile(tile: Tile, opts: &Options, buf: &mut Vec<Rgb>) {
    let offset_x = tile.i * TILE_WIDTH;
    let offset_y = tile.j * TILE_HEIGHT;

    //println!("tile = <{}, {}>", tile.i, tile.j);

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
    let opts = Options {
        width_in_pixels: 640,
        height_in_pixels: 480,
        iterations: 256,
        window: Window {
            x0: -2.0, x1: 1.0,
            y0: -1.0, y1: 1.0
        }
    };

    let num_pixels = (opts.width_in_pixels * opts.height_in_pixels) as uint;
    let mut buf: Vec<Rgb> = Vec::from_elem(num_pixels, Rgb{r: 0.0, g: 0.0, b: 0.0});

    mandelbrot(&opts, &mut buf);
    write(&opts, &buf);
}
