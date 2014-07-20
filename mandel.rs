struct Window {
    x0: f32,
    x1: f32,
    y0: f32,
    y1: f32
}

fn mandel(c_real: f32, c_imag: f32, depth: int) -> int {
    let mut z_real = c_real;
    let mut z_imag = c_imag;
    let mut iter = 0i;

    for i in range(0i, depth) {
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

fn shade(x: f32, y: f32, w:f32, h: f32, val: int, max: int) -> (int, int, int) {
    let r0 = (x / w * max as f32) as int;
    let g0 = (y / h * max as f32) as int;
    let b0 = (0.85f32 * (r0 + g0) as f32) as int % max;

    let rf0 = r0 as f32 / max as f32;
    let gf0 = g0 as f32 / max as f32;
    let bf0 = b0 as f32 / max as f32;

    let t = val as f32 / max as f32;

    let r = (((1.0 - rf0) * t + rf0) * max as f32) as int;
    let g = (((1.0 - gf0) * t + gf0) * max as f32) as int;
    let b = (((1.0 - bf0) * t + bf0) * max as f32) as int;

    if r == 255 && g == 255 && b == 255 {
        return (0, 0, 0);
    }

    (r, g, b)
}

fn single_threaded_mandelbrot(width: int, height: int, depth: int, window: Window) {
    let image_width = width as f32;
    let image_height = height as f32;
    let window_width = window.x1 - window.x0;
    let window_height = window.y1 - window.y0;

    for j in range(0i, height) {
        let y = (j as f32 + 0.5f32) / image_height * window_height + window.y0;
        for i in range(0i, width) {
            let x = (i as f32 + 0.5f32) / image_width * window_width + window.x0;
            let val = mandel(x, y, depth);
            let (r, g, b) = shade(x, y, window_width, window_height, val, depth);
            print!("{} {} {} ", r, g, b);
        }
        print!("\n");
    }
}

fn main() {
    let width = 640i;
    let height = 480i;
    let depth = 256i;
    let window = Window {x0: -2.0, x1: 1.0, y0: -1.0, y1: 1.0};

    println!("P3 {} {} {}", width, height, depth);

    single_threaded_mandelbrot(width, height, depth, window);
}
