<?hh // strict
namespace Usox\HackMock;

use Facebook\HackCodegen\CodegenClass;
use Facebook\HackCodegen\HackCodegenFactory;
use Facebook\HackCodegen\HackCodegenConfig;
use HH\Lib\{Str, Dict, C};

final class Mock<TC> implements MockInterface {

	private dict<string, ExpectationInterface> $expectations = dict[];

	private HackCodegenFactory $code_generator;

	private static Map<string, mixed> $registry = Map{};

	public function __construct(private classname<TC> $interface_name): void {
		$this->code_generator = new HackCodegenFactory(new HackCodegenConfig());
	}

	public function expects(string $method_name): ExpectationInterface {
		$expectation = new Expectation($method_name);
		$this->expectations[$method_name] = $expectation;
		return $expectation;
	}

	public function build(): TC {
		$rfl = new \ReflectionClass($this->interface_name);

		$mock_name = Str\format(
			'%s_Implementation%s',
			$rfl->getShortName(),
			\spl_object_hash($this)
		);

		$class = $this->code_generator->codegenClass($mock_name)
			->addInterface(
				$this->code_generator->codegenImplementsInterface($rfl->getName())
			)
			->addEmptyUserAttribute('__MockClass');

			foreach ($rfl->getMethods() as $method) {
			$method_name = $method->getName();

			$gen_method = $this
				->code_generator
				->codegenMethod(
					$method_name
				)
				->setReturnType('mixed');

			foreach ($method->getParameters() as $parameter) {
				$gen_method->addParameterf(
					'%s $%s',
					$parameter->getTypehintText(),
					$parameter->getName(),
				);
			}

			$class->addMethod($gen_method);

			if (C\contains_key($this->expectations, $method_name)) {
				$expectation = $this->expectations[$method_name];

				$gen_method->setBodyf(
					'return \%s::getRegistry()[\'%s\'] ?? null;',
					__CLASS__,
					$expectation->getMethodName()
				);

				continue;
			}
			$gen_method->setBodyf(
				'return null;'
			);
		}


		// UNSAFE
		eval($class->render());

		return new $mock_name();
	}

	public static function getRegistry(): Map<string, mixed> {
		return static::$registry;
	}

	private function addMethod(CodegenClass $class, ExpectationInterface $expectation): void {
		$class->addMethod(
			$this->code_generator->codegenMethod(
				$expectation->getMethodName()
			)
			->addParameter('mixed ...$params')
			->setReturnType(
				$expectation->getReturnType()
			)
			->setBodyf(
				'return \%s::getRegistry()[\'%s\'];',
				__CLASS__,
				$expectation->getMethodName()
			)
		);
	}
}
