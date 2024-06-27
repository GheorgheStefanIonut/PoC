import os
import logging
from logging.handlers import RotatingFileHandler
from flask import request

def setup_logger():
    """
    Sets up the logger for the Flask application and returns a decorator for logging.

    Returns:
        function: A decorator function for logging requests and responses.
    """
    log_directory = os.path.join(os.path.dirname(__file__), 'log')
    os.makedirs(log_directory, exist_ok=True)
    log_file = os.path.join(log_directory, 'api.log')

    log_handler = RotatingFileHandler(log_file, maxBytes=10000, backupCount=3)
    log_handler.setLevel(logging.INFO)
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    log_handler.setFormatter(formatter)

    def decorator(app):
        app.logger.addHandler(log_handler)
        app.logger.setLevel(logging.INFO)

        @app.before_request
        def log_request_info():
            app.logger.info(f'Request: {request.method} {request.url} - Body: {request.get_data()}')

        @app.after_request
        def log_response_info(response):
            app.logger.info(f'Response: {response.status} {response.get_data()}')
            return response
        
        return app

    return decorator

from functools import wraps

def logger_frame(log_file):
    """
    Custom logger decorator that logs function calls and arguments to a specified log file.

    Args:
        log_file (str): Name of the log file where the logs will be written (e.g., 'generate_frame.log').
    """
    log_directory = os.path.join(os.path.dirname(__file__), 'log')
    os.makedirs(log_directory, exist_ok=True)
    log_path = os.path.join(log_directory, log_file)

    log_handler = logging.FileHandler(log_path)
    log_handler.setLevel(logging.INFO)
    formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
    log_handler.setFormatter(formatter)

    def decorator(cls):
        for attr_name in dir(cls):
            attr = getattr(cls, attr_name)
            if callable(attr) and not attr_name.startswith('__'):
                setattr(cls, attr_name, log_method_calls(attr, log_handler))
        return cls

    return decorator

def log_method_calls(method, handler):
    """
    Wrapper function to log method calls and arguments.

    Args:
        method (function): Method of the class to be wrapped.
        handler (logging.Handler): Logging handler to log messages.

    Returns:
        function: Wrapped function that logs method calls.
    """
    @wraps(method)
    def wrapper(self, *args, **kwargs):
        args_str = ', '.join([repr(a) for a in args])
        kwargs_str = ', '.join([f'{k}={repr(v)}' for k, v in kwargs.items()])
        arguments_str = ', '.join(filter(None, [args_str, kwargs_str]))
        log_message = f'Function "{method.__name__}" called with arguments: {arguments_str}'
        handler.emit(logging.LogRecord(
            name=method.__module__,
            level=logging.INFO,
            pathname=None,
            lineno=None,
            msg=log_message,
            args=None,
            exc_info=None,
        ))
        return method(self, *args, **kwargs)

    return wrapper