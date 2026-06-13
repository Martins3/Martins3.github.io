use candle_core::Tensor;
use teach_core::DeviceKind;
use teach_model::resolve_candle_device;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    eprintln!("[cuda-smoke] starting");

    let device = resolve_candle_device(DeviceKind::Cuda)?;
    eprintln!("[cuda-smoke] device created: {:?}", device);

    let tensor = Tensor::new(&[1f32, 2.0, 3.0], &device)?;
    eprintln!("[cuda-smoke] tensor created");

    let sum = tensor.sum_all()?.to_scalar::<f32>()?;
    eprintln!("[cuda-smoke] sum={sum}");

    Ok(())
}
